#ifndef COMPILE_SERVICE_CLIENT_H
#define COMPILE_SERVICE_CLIENT_H

#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "compile_service_protocol.h"

namespace compile_service {

struct ParsedCompileServiceClientArgs {
  bool use_explicit_id = false;
  std::string config_or_id;
  std::string target;
  std::string op = "compile";
};

inline std::string format_compile_service_usage() {
  return "Usage: lpccp <config-path> <path>\n"
         "   or: lpccp --dev-test <config-path> <path>\n"
         "   or: lpccp --dev-test --id <id> <path>\n"
         "   or: lpccp --id <id> <path>\n";
}

inline bool parse_compile_service_client_args(const std::vector<std::string_view> &argv,
                                              ParsedCompileServiceClientArgs *out) {
  if (argv.size() == 2) {
    if (argv[0] == "--id" || argv[0] == "--dev-test") {
      return false;
    }
    out->config_or_id = argv[0];
    out->target = argv[1];
    return true;
  }

  if (argv.size() == 3 && argv[0] == "--id") {
    out->use_explicit_id = true;
    out->config_or_id = argv[1];
    out->target = argv[2];
    return true;
  }

  if (argv.size() == 3 && argv[0] == "--dev-test") {
    if (argv[1] == "--id") {
      return false;
    }
    out->op = "dev_test";
    out->config_or_id = argv[1];
    out->target = argv[2];
    return true;
  }

  if (argv.size() == 4 && argv[0] == "--dev-test" && argv[1] == "--id") {
    out->op = "dev_test";
    out->use_explicit_id = true;
    out->config_or_id = argv[2];
    out->target = argv[3];
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
