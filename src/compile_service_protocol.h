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
  std::string mode = "reload_loaded";
};

struct CompileServiceResponse {
  int version = 1;
  bool ok = false;
  std::string kind;
  std::string target;
  std::string phase;
  std::string reason;
  std::string message;
  int files_total = 0;
  int files_ok = 0;
  int files_failed = 0;
  std::vector<CompileServiceDiagnostic> diagnostics;
  std::vector<nlohmann::json> runtime_errors;
  std::vector<nlohmann::json> results;
  std::vector<std::string> output;
  nlohmann::json result;
  nlohmann::json error;
  nlohmann::json summary;
  std::string compile_status;
  std::string test_status;
  std::string test_message;
};

inline void set_compile_service_failure(CompileServiceResponse *response,
                                        std::string_view phase,
                                        std::string_view reason,
                                        std::string message) {
  response->ok = false;
  response->phase = std::string(phase);
  response->reason = std::string(reason);
  response->message = std::move(message);
}

inline void add_compile_service_diagnostic(CompileServiceResponse *response,
                                           std::string_view severity,
                                           std::string_view file,
                                           int line,
                                           int column,
                                           std::string message) {
  response->diagnostics.push_back(CompileServiceDiagnostic{
      std::string(severity), std::string(file), line, column, std::move(message)});
}

inline CompileServiceResponse build_compile_queue_timeout_response(std::string_view kind,
                                                                  std::string_view target,
                                                                  int timeout_ms) {
  CompileServiceResponse response;
  response.version = 1;
  response.ok = false;
  response.kind = std::string(kind);
  response.target = std::string(target);
  set_compile_service_failure(&response, "connect", "compile_timeout",
                              "compile request timed out in queue after " +
                                  std::to_string(timeout_ms) + "ms");
  add_compile_service_diagnostic(&response, "error",
                                 response.kind == "file" ? response.target : std::string{},
                                 0, 0, response.message);
  return response;
}

inline CompileServiceResponse build_dev_test_queue_timeout_response(std::string_view target,
                                                                   int timeout_ms) {
  CompileServiceResponse response;
  response.version = 1;
  response.ok = false;
  response.kind = "dev_test";
  response.target = std::string(target);
  set_compile_service_failure(&response, "connect", "compile_timeout",
                              "dev_test request timed out in queue after " +
                                  std::to_string(timeout_ms) + "ms");
  response.compile_status = "unknown";
  response.test_status = "not_run";
  response.test_message = response.message;
  response.error = nlohmann::json{{"type", "queue_timeout"},
                                  {"message", "dev_test request timed out in queue after " +
                                                  std::to_string(timeout_ms) + "ms"}};
  return response;
}

inline CompileServiceResponse build_client_failure_response(std::string_view kind,
                                                           std::string_view target,
                                                           std::string_view pipe_name,
                                                           std::string_view phase,
                                                           std::string_view reason,
                                                           std::string message,
                                                           unsigned long error_code) {
  CompileServiceResponse response;
  response.version = 1;
  response.ok = false;
  response.kind = std::string(kind);
  response.target = std::string(target);
  set_compile_service_failure(&response, phase, reason, std::move(message));
  response.error = nlohmann::json{{"type", response.reason},
                                  {"message", response.message},
                                  {"pipe_name", pipe_name},
                                  {"win32", error_code}};
  add_compile_service_diagnostic(&response, "error", "", 0, 0, response.message);
  return response;
}

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
  j = nlohmann::json{{"version", value.version},
                     {"op", value.op},
                     {"target", value.target},
                     {"mode", value.mode}};
}

inline void from_json(const nlohmann::json &j, CompileServiceRequest &value) {
  j.at("version").get_to(value.version);
  j.at("op").get_to(value.op);
  j.at("target").get_to(value.target);
  if (j.contains("mode")) {
    j.at("mode").get_to(value.mode);
  }
}

inline void to_json(nlohmann::json &j, const CompileServiceResponse &value) {
  j = nlohmann::json{{"version", value.version},
                     {"ok", value.ok},
                     {"kind", value.kind},
                     {"target", value.target},
                     {"phase", value.phase},
                     {"reason", value.reason},
                     {"message", value.message},
                     {"files_total", value.files_total},
                     {"files_ok", value.files_ok},
                     {"files_failed", value.files_failed},
                     {"diagnostics", value.diagnostics},
                     {"runtime_errors", value.runtime_errors},
                     {"results", value.results},
                     {"output", value.output}};
  if (!value.result.is_null()) {
    j["result"] = value.result;
  }
  if (!value.error.is_null()) {
    j["error"] = value.error;
  }
  if (!value.summary.is_null()) {
    j["summary"] = value.summary;
  }
  if (!value.compile_status.empty()) {
    j["compile_status"] = value.compile_status;
  }
  if (!value.test_status.empty()) {
    j["test_status"] = value.test_status;
  }
  if (!value.test_message.empty()) {
    j["test_message"] = value.test_message;
  }
}

inline void from_json(const nlohmann::json &j, CompileServiceResponse &value) {
  j.at("version").get_to(value.version);
  j.at("ok").get_to(value.ok);
  j.at("kind").get_to(value.kind);
  j.at("target").get_to(value.target);
  if (j.contains("phase")) {
    j.at("phase").get_to(value.phase);
  }
  if (j.contains("reason")) {
    j.at("reason").get_to(value.reason);
  }
  if (j.contains("message")) {
    j.at("message").get_to(value.message);
  }
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
  if (j.contains("runtime_errors")) {
    j.at("runtime_errors").get_to(value.runtime_errors);
  }
  if (j.contains("results")) {
    j.at("results").get_to(value.results);
  }
  if (j.contains("output")) {
    j.at("output").get_to(value.output);
  }
  if (j.contains("result")) {
    value.result = j.at("result");
  }
  if (j.contains("error")) {
    value.error = j.at("error");
  }
  if (j.contains("summary")) {
    value.summary = j.at("summary");
  }
  if (j.contains("compile_status")) {
    j.at("compile_status").get_to(value.compile_status);
  }
  if (j.contains("test_status")) {
    j.at("test_status").get_to(value.test_status);
  }
  if (j.contains("test_message")) {
    j.at("test_message").get_to(value.test_message);
  }
}

}  // namespace compile_service

#endif  // COMPILE_SERVICE_PROTOCOL_H
