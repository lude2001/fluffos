#include "base/std.h"

#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "compile_service_protocol.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
struct ParsedArgs {
  bool use_explicit_id = false;
  std::string config_or_id;
  std::string target;
};

void print_usage() {
  std::cerr << "Usage: lpccp <config-path> <path>\n"
               "   or: lpccp --id <id> <path>\n";
}

bool parse_args(int argc, char **argv, ParsedArgs *out) {
  if (argc == 3) {
    out->config_or_id = argv[1];
    out->target = argv[2];
    return true;
  }

  if (argc == 4 && std::string_view(argv[1]) == "--id") {
    out->use_explicit_id = true;
    out->config_or_id = argv[2];
    out->target = argv[3];
    return true;
  }

  return false;
}

nlohmann::json build_stub_response(const ParsedArgs &args, const std::string &id) {
  nlohmann::json j;
  j["version"] = 1;
  j["ok"] = false;
  j["kind"] = "client_stub";
  j["target"] = args.target;
  j["id"] = id;
  j["pipe_name"] = compile_service::make_compile_service_pipe_name(id);
  if (!args.use_explicit_id) {
    j["config_path"] = compile_service::normalize_compile_service_path(args.config_or_id);
  }
  auto diagnostics = nlohmann::json::array();
  diagnostics.push_back(
      {{"severity", "info"},
       {"file", args.use_explicit_id
                    ? std::string{}
                    : compile_service::normalize_compile_service_path(args.config_or_id)},
       {"line", 0},
       {"message", "lpccp transport stub"}});
  j["diagnostics"] = diagnostics;
  return j;
}

#ifdef _WIN32
bool can_connect_to_pipe(const std::string &pipe_name) {
  HANDLE pipe = CreateFileA(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                            0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) {
    return false;
  }
  CloseHandle(pipe);
  return true;
}

void print_pipe_connect_error(const std::string &pipe_name) {
  auto error = GetLastError();
  std::cerr << "Error: cannot connect to compile service pipe " << pipe_name << " (win32=" << error
            << ")\n";
}
#else
bool can_connect_to_pipe(const std::string &) { return false; }

void print_pipe_connect_error(const std::string &pipe_name) {
  std::cerr << "Error: named-pipe transport is only available on Windows for " << pipe_name << '\n';
}
#endif
}  // namespace

int main(int argc, char **argv) {
  ParsedArgs args;
  if (!parse_args(argc, argv, &args)) {
    print_usage();
    return 1;
  }

  const std::string id =
      args.use_explicit_id ? args.config_or_id : compile_service::make_compile_service_id(args.config_or_id);
  const auto pipe_name = compile_service::make_compile_service_pipe_name(id);
  if (!can_connect_to_pipe(pipe_name)) {
    print_pipe_connect_error(pipe_name);
    return 2;
  }

  const auto response = build_stub_response(args, id);
  std::cout << response.dump(2) << '\n';
  return 0;
}
