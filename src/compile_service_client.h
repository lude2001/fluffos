#ifndef COMPILE_SERVICE_CLIENT_H
#define COMPILE_SERVICE_CLIENT_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "compile_service_protocol.h"

namespace compile_service {

struct ParsedCompileServiceClientArgs {
  bool use_explicit_id = false;
  std::string config_or_id;
  std::string target;
  std::string op = "compile";
  std::string mode = "reload_loaded";
};

inline std::string format_compile_service_usage() {
  return "Usage: lpccp <config-path> <path>\n"
         "   or: lpccp [--compile-only|--reload-loaded|--fresh-required] <config-path> <path>\n"
         "   or: lpccp --dev-test <config-path> <path>\n"
         "   or: lpccp --dev-test --id <id> <path>\n"
         "   or: lpccp --id <id> <path>\n";
}

inline bool parse_compile_service_client_args(const std::vector<std::string_view> &argv,
                                              ParsedCompileServiceClientArgs *out) {
  ParsedCompileServiceClientArgs parsed;
  std::vector<std::string_view> positional;

  for (size_t i = 0; i < argv.size(); i++) {
    auto arg = argv[i];
    if (arg == "--dev-test") {
      parsed.op = "dev_test";
      continue;
    }
    if (arg == "--compile-only") {
      parsed.mode = "compile_only";
      continue;
    }
    if (arg == "--reload-loaded") {
      parsed.mode = "reload_loaded";
      continue;
    }
    if (arg == "--fresh-required") {
      parsed.mode = "fresh_required";
      continue;
    }
    if (arg == "--id") {
      if (parsed.use_explicit_id || i + 1 >= argv.size()) {
        return false;
      }
      parsed.use_explicit_id = true;
      parsed.config_or_id = argv[++i];
      continue;
    }
    if (!arg.empty() && arg[0] == '-') {
      return false;
    }
    positional.push_back(arg);
  }

  if (parsed.use_explicit_id) {
    if (positional.size() != 1) {
      return false;
    }
    parsed.target = positional[0];
    *out = std::move(parsed);
    return true;
  }

  if (positional.size() == 2) {
    parsed.config_or_id = positional[0];
    parsed.target = positional[1];
    *out = std::move(parsed);
    return true;
  }

  return false;
}

inline std::string format_compile_service_connect_error(std::string_view pipe_name,
                                                        unsigned long error_code) {
  return "Error: cannot connect to compile service pipe " + std::string(pipe_name) + " (win32=" +
         std::to_string(error_code) + ")\n";
}

inline nlohmann::json build_compile_service_stub_response(
    const ParsedCompileServiceClientArgs &args, const std::string &id) {
  nlohmann::json j;
  j["version"] = 1;
  j["ok"] = false;
  j["kind"] = "client_stub";
  j["target"] = args.target;
  j["phase"] = "connect";
  j["reason"] = "client_stub";
  j["message"] = "lpccp transport stub";
  j["id"] = id;
  j["pipe_name"] = make_compile_service_pipe_name(id);
  if (!args.use_explicit_id) {
    j["config_path"] = normalize_compile_service_path(args.config_or_id);
  }

  auto diagnostics = nlohmann::json::array();
  diagnostics.push_back({{"severity", "info"},
                         {"file", args.use_explicit_id ? std::string{}
                                                       : normalize_compile_service_path(args.config_or_id)},
                         {"line", 0},
                         {"message", "lpccp transport stub"}});
  j["diagnostics"] = diagnostics;
  return j;
}

}  // namespace compile_service

#endif  // COMPILE_SERVICE_CLIENT_H
