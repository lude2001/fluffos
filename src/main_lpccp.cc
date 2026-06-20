#include "base/std.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "compile_service_client.h"
#include "compile_service_protocol.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
void print_usage() {
  std::cerr << compile_service::format_compile_service_usage();
}

#ifdef _WIN32
std::string compile_kind_from_target(std::string_view target) {
  if (!target.empty() && (target.back() == '/' || target.back() == '\\')) {
    return "directory";
  }
  auto dot = target.find_last_of('.');
  if (dot != std::string_view::npos && target.substr(dot) == ".h") {
    return "header";
  }
  return "file";
}

struct PipeOpenResult {
  HANDLE pipe = INVALID_HANDLE_VALUE;
  DWORD error = 0;
  bool timed_out = false;
  bool saw_busy = false;
};

PipeOpenResult open_pipe_with_retry(const std::string &pipe_name) {
  PipeOpenResult result;
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  DWORD wait_ms = 25;

  while (std::chrono::steady_clock::now() < deadline) {
    result.pipe =
        CreateFileA(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (result.pipe != INVALID_HANDLE_VALUE) {
      result.error = 0;
      return result;
    }

    result.error = GetLastError();
    if (result.error == ERROR_PIPE_BUSY) {
      result.saw_busy = true;
      WaitNamedPipeA(pipe_name.c_str(), wait_ms);
    } else if (result.error != ERROR_FILE_NOT_FOUND && result.error != ERROR_PATH_NOT_FOUND) {
      std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
    }
    wait_ms = std::min<DWORD>(wait_ms * 2, 250);
  }

  result.timed_out = true;
  return result;
}

int print_client_failure(const compile_service::ParsedCompileServiceClientArgs &args,
                         const std::string &pipe_name,
                         std::string_view reason,
                         std::string message,
                         unsigned long error_code) {
  auto response = compile_service::build_client_failure_response(
      compile_kind_from_target(args.target), args.target, pipe_name, "connect", reason,
      std::move(message), error_code);
  std::cout << nlohmann::json(response).dump(2) << '\n';
  return 2;
}
#endif

}  // namespace

int main(int argc, char **argv) {
  compile_service::ParsedCompileServiceClientArgs args;
  std::vector<std::string_view> argv_view;
  argv_view.reserve(argc > 0 ? static_cast<size_t>(argc - 1) : 0);
  for (int i = 1; i < argc; i++) {
    argv_view.emplace_back(argv[i]);
  }

  if (!compile_service::parse_compile_service_client_args(argv_view, &args)) {
    print_usage();
    return 1;
  }

  const std::string id =
      args.use_explicit_id ? args.config_or_id : compile_service::make_compile_service_id(args.config_or_id);
  const auto pipe_name = compile_service::make_compile_service_pipe_name(id);
  auto request = compile_service::CompileServiceRequest{};
  request.op = args.op;
  request.target = args.target;
  request.mode = args.mode;

#ifdef _WIN32
  auto open_result = open_pipe_with_retry(pipe_name);
  HANDLE pipe = open_result.pipe;
  if (pipe == INVALID_HANDLE_VALUE) {
    auto final_error = open_result.error ? open_result.error : GetLastError();
    auto reason = open_result.saw_busy ? "service_busy"
                                       : (final_error == ERROR_SEM_TIMEOUT ? "timeout"
                                                                           : "pipe_connect_failed");
    auto message = compile_service::format_compile_service_connect_error(
        pipe_name, final_error);
    return print_client_failure(args, pipe_name, reason, message, final_error);
  }

  auto payload = nlohmann::json(request).dump() + "\n";
  DWORD bytes_written = 0;
  if (!WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &bytes_written, nullptr)) {
    auto error = GetLastError();
    CloseHandle(pipe);
    return print_client_failure(args, pipe_name, "pipe_connect_failed",
                                "Error: failed to write compile service request to pipe " + pipe_name +
                                    " (win32=" + std::to_string(error) + ")\n",
                                error);
  }

  std::string response_text;
  char buffer[4096];
  DWORD bytes_read = 0;
  while (ReadFile(pipe, buffer, sizeof buffer, &bytes_read, nullptr) && bytes_read > 0) {
    response_text.append(buffer, buffer + bytes_read);
    auto pos = response_text.find('\n');
    if (pos != std::string::npos) {
      response_text.resize(pos);
      break;
    }
  }
  CloseHandle(pipe);

  if (response_text.empty()) {
    return print_client_failure(args, pipe_name, "pipe_connect_failed",
                                "Error: compile service closed pipe without a response " + pipe_name + "\n",
                                0);
  }

  auto response_json = nlohmann::json::parse(response_text);
  std::cout << response_json.dump(2) << '\n';
  if (response_json.value("ok", false)) {
    return 0;
  }
  return 1;
#else
  std::cerr << compile_service::format_compile_service_connect_error(pipe_name, 0);
  return 2;
#endif
}
