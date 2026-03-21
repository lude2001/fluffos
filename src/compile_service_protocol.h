#ifndef COMPILE_SERVICE_PROTOCOL_H
#define COMPILE_SERVICE_PROTOCOL_H

#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace compile_service {

struct CompileServiceDiagnostic {
  std::string severity;
  std::string file;
  int line = 0;
  std::string message;
};

struct CompileServiceRequest {
  int version = 1;
  std::string op = "compile";
  std::string target;
};

struct CompileServiceResponse {
  int version = 1;
  bool ok = false;
  std::string kind;
  std::string target;
  std::vector<CompileServiceDiagnostic> diagnostics;
};

inline std::string normalize_compile_service_path(std::string_view path) {
  std::filesystem::path p(path);
  std::error_code ec;
  auto absolute = std::filesystem::absolute(p, ec);
  if (!ec) {
    return absolute.lexically_normal().generic_string();
  }
  return p.lexically_normal().generic_string();
}

inline std::uint64_t fnv1a_64(std::string_view text) {
  constexpr std::uint64_t kOffsetBasis = 14695981039346656037ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;

  std::uint64_t hash = kOffsetBasis;
  for (unsigned char c : text) {
    hash ^= c;
    hash *= kPrime;
  }
  return hash;
}

inline std::string to_hex_64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

inline std::string make_compile_service_id(std::string_view config_path) {
  return to_hex_64(fnv1a_64(normalize_compile_service_path(config_path)));
}

inline std::string make_compile_service_pipe_name(std::string_view id) {
  return std::string("\\\\.\\pipe\\fluffos-lpccp-") + std::string(id);
}

}  // namespace compile_service

namespace compile_service {

inline void to_json(nlohmann::json &j, const CompileServiceDiagnostic &value) {
  j = nlohmann::json{{"severity", value.severity},
                     {"file", value.file},
                     {"line", value.line},
                     {"message", value.message}};
}

inline void from_json(const nlohmann::json &j, CompileServiceDiagnostic &value) {
  j.at("severity").get_to(value.severity);
  j.at("file").get_to(value.file);
  j.at("line").get_to(value.line);
  j.at("message").get_to(value.message);
}

inline void to_json(nlohmann::json &j, const CompileServiceRequest &value) {
  j = nlohmann::json{{"version", value.version}, {"op", value.op}, {"target", value.target}};
}

inline void from_json(const nlohmann::json &j, CompileServiceRequest &value) {
  j.at("version").get_to(value.version);
  j.at("op").get_to(value.op);
  j.at("target").get_to(value.target);
}

inline void to_json(nlohmann::json &j, const CompileServiceResponse &value) {
  j = nlohmann::json{{"version", value.version},
                     {"ok", value.ok},
                     {"kind", value.kind},
                     {"target", value.target},
                     {"diagnostics", value.diagnostics}};
}

inline void from_json(const nlohmann::json &j, CompileServiceResponse &value) {
  j.at("version").get_to(value.version);
  j.at("ok").get_to(value.ok);
  j.at("kind").get_to(value.kind);
  j.at("target").get_to(value.target);
  j.at("diagnostics").get_to(value.diagnostics);
}

}  // namespace compile_service

#endif  // COMPILE_SERVICE_PROTOCOL_H
