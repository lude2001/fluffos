#ifndef COMPILE_SERVICE_PROTOCOL_H
#define COMPILE_SERVICE_PROTOCOL_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <system_error>
#include <string>
#include <string_view>
#include <vector>

namespace compile_service {

struct CompileServiceDiagnostic {
  std::string severity;
  std::string file;
  int line = 0;
  int column = 0;
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
  int files_total = 0;
  int files_ok = 0;
  int files_failed = 0;
  std::vector<CompileServiceDiagnostic> diagnostics;
  std::vector<nlohmann::json> results;
};

inline std::string normalize_compile_service_path(std::string_view path) {
  std::filesystem::path p(path);
  std::error_code ec;
  auto absolute = std::filesystem::absolute(p, ec);
  std::filesystem::path normalized = !ec ? absolute.lexically_normal() : p.lexically_normal();

  ec.clear();
  auto canonical = std::filesystem::weakly_canonical(normalized, ec);
  if (!ec) {
    normalized = canonical;
  }

  auto result = normalized.generic_string();
#ifdef _WIN32
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
#endif
  return result;
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
  if (value.column > 0) {
    j["column"] = value.column;
  }
}

inline void from_json(const nlohmann::json &j, CompileServiceDiagnostic &value) {
  j.at("severity").get_to(value.severity);
  j.at("file").get_to(value.file);
  j.at("line").get_to(value.line);
  if (j.contains("column")) {
    j.at("column").get_to(value.column);
  }
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
                     {"files_total", value.files_total},
                     {"files_ok", value.files_ok},
                     {"files_failed", value.files_failed},
                     {"diagnostics", value.diagnostics},
                     {"results", value.results}};
}

inline void from_json(const nlohmann::json &j, CompileServiceResponse &value) {
  j.at("version").get_to(value.version);
  j.at("ok").get_to(value.ok);
  j.at("kind").get_to(value.kind);
  j.at("target").get_to(value.target);
  if (j.contains("files_total")) {
    j.at("files_total").get_to(value.files_total);
  }
  if (j.contains("files_ok")) {
    j.at("files_ok").get_to(value.files_ok);
  }
  if (j.contains("files_failed")) {
    j.at("files_failed").get_to(value.files_failed);
  }
  if (j.contains("diagnostics")) {
    j.at("diagnostics").get_to(value.diagnostics);
  }
  if (j.contains("results")) {
    j.at("results").get_to(value.results);
  }
}

}  // namespace compile_service

#endif  // COMPILE_SERVICE_PROTOCOL_H
