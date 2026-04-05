#include "base/std.h"

#include <iostream>
#include <string>
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

#ifdef _WIN32
  HANDLE pipe =
      CreateFileA(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) {
    std::cerr << compile_service::format_compile_service_connect_error(pipe_name, GetLastError());
    return 2;
  }

  auto payload = nlohmann::json(request).dump() + "\n";
  DWORD bytes_written = 0;
  if (!WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &bytes_written, nullptr)) {
    std::cerr << compile_service::format_compile_service_connect_error(pipe_name, GetLastError());
    CloseHandle(pipe);
    return 2;
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
    auto response = compile_service::build_compile_service_stub_response(args, id);
    std::cout << response.dump(2) << '\n';
    return 2;
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
