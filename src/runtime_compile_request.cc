#include "runtime_compile_request.h"
#include "runtime_dev_test_request.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "compiler/internal/compiler.h"
#include "vm/internal/base/interpret.h"
#include "vm/internal/base/object.h"
#include "vm/internal/base/program.h"
#include "vm/internal/master.h"
#include "vm/internal/simulate.h"
#include "vm/internal/simul_efun.h"
#include "vm/internal/base/machine.h"
#include "vm/vm.h"

namespace compile_service {
namespace {

std::string normalize_lpc_target(std::string target) {
  if (target.empty()) {
    return target;
  }

  std::replace(target.begin(), target.end(), '\\', '/');
  if (target[0] != '/') {
    target.insert(target.begin(), '/');
  }
  return target;
}

std::filesystem::path target_to_filesystem_path(const std::string &target) {
  auto normalized = normalize_lpc_target(target);
  if (!normalized.empty() && normalized[0] == '/') {
    normalized.erase(normalized.begin());
  }
  return std::filesystem::path(normalized);
}

bool has_extension(std::string_view target, std::string_view extension) {
  if (target.size() < extension.size()) {
    return false;
  }
  return std::equal(extension.rbegin(), extension.rend(), target.rbegin(),
                    [](char lhs, char rhs) {
                      return static_cast<char>(std::tolower(static_cast<unsigned char>(lhs))) ==
                             static_cast<char>(std::tolower(static_cast<unsigned char>(rhs)));
                    });
}

std::string current_error_message() {
  if (catch_value.type == T_STRING && catch_value.u.string) {
    return catch_value.u.string;
  }
  return "runtime error";
}

nlohmann::json make_runtime_error(std::string_view message) {
  nlohmann::json error;
  error["message"] = std::string(message);
  error["error_type"] = "runtime_error";
  if (current_object) {
    error["object"] = std::string("/") + current_object->obname;
  }
  if (current_prog) {
    error["program"] = std::string("/") + current_prog->filename;
    const char *file = nullptr;
    int line = 0;
    get_line_number_info(&file, &line);
    if (file) {
      error["file"] = add_slash(file);
    }
    if (line > 0) {
      error["line"] = line;
    }
  }
  error["trace"] = nlohmann::json::array();
  return error;
}

std::string classify_failed_compile(const CompileServiceResponse &response, bool had_existing_object) {
  const bool has_compile_error =
      std::any_of(response.diagnostics.begin(), response.diagnostics.end(),
                  [](const CompileServiceDiagnostic &diagnostic) {
                    return diagnostic.severity == "error";
                  });
  if (has_compile_error) {
    return "syntax_error";
  }
  if (had_existing_object && !response.runtime_errors.empty()) {
    return "reload_loaded_object_failed";
  }
  if (!response.runtime_errors.empty()) {
    return "runtime_error";
  }
  if (had_existing_object) {
    return "reload_loaded_object_failed";
  }
  return "service_error";
}

std::string phase_for_reason(std::string_view reason) {
  if (reason == "syntax_error") {
    return "compile";
  }
  if (reason == "reload_loaded_object_failed") {
    return "reload";
  }
  if (reason == "runtime_error") {
    return "runtime";
  }
  return "compile";
}

void finalize_failed_compile(CompileServiceResponse *response, bool had_existing_object) {
  auto reason = classify_failed_compile(*response, had_existing_object);
  auto phase = phase_for_reason(reason);
  auto message = response->runtime_errors.empty()
                     ? current_error_message()
                     : response->runtime_errors.back().value("message", "runtime error");
  if (reason == "syntax_error" && !response->diagnostics.empty()) {
    message = response->diagnostics.front().message;
  } else if (reason == "reload_loaded_object_failed") {
    message = "failed to reload loaded object " + response->target + ": " + message;
  }
  set_compile_service_failure(response, phase, reason, message);
  if (response->diagnostics.empty()) {
    add_compile_service_diagnostic(response, "error", response->target, 0, 0, response->message);
  }
}

CompileServiceResponse unsupported_target_response(const std::string &target,
                                                   std::string_view kind,
                                                   std::string message) {
  CompileServiceResponse response;
  response.version = 1;
  response.kind = std::string(kind);
  response.target = target;
  set_compile_service_failure(&response, "target", "unsupported_target_kind", std::move(message));
  add_compile_service_diagnostic(&response, "error", target, 0, 0, response.message);
  return response;
}

CompileServiceResponse target_not_found_response(const std::string &target) {
  CompileServiceResponse response;
  response.version = 1;
  response.kind = "file";
  response.target = target;
  set_compile_service_failure(&response, "target", "target_not_found",
                              "target source file was not found: " + target);
  add_compile_service_diagnostic(&response, "error", target, 0, 0, response.message);
  return response;
}

CompileServiceResponse compile_single_file(const std::string &target, std::string_view mode) {
  CompileServiceResponse response;
  response.version = 1;
  response.kind = "file";
  response.target = normalize_lpc_target(target);
  response.files_total = 1;

  if (has_extension(response.target, ".h")) {
    return unsupported_target_response(response.target, "header",
                                       "header files are not standalone compile targets");
  }

  std::vector<CompileServiceDiagnostic> diagnostics;
  std::vector<nlohmann::json> runtime_errors;
  set_compile_service_diagnostics_collector(&diagnostics);

  object_t *result = nullptr;
  object_t *previous_object = current_object;
  bool had_existing_object = false;
  error_context_t econ{};
  save_context(&econ);
  push_runtime_error_sink([&runtime_errors](std::string_view message) {
    runtime_errors.push_back(make_runtime_error(message));
  });
  try {
    current_object = master_ob;
    if (auto *existing = find_object2(response.target.c_str())) {
      had_existing_object = true;
      if (mode == "fresh_required") {
        set_compile_service_failure(
            &response, "reload", "reload_loaded_object_failed",
            "target is already loaded in this runtime; restart the runtime for a fresh compile");
        add_compile_service_diagnostic(&response, "error", response.target, 0, 0,
                                       response.message);
        result = nullptr;
        throw "compile_service fresh_required";
      }
      if (mode == "compile_only") {
        set_compile_service_failure(
            &response, "compile", "reload_loaded_object_failed",
            "compile-only cannot validate an already-loaded object in this runtime; restart the runtime or use --reload-loaded");
        add_compile_service_diagnostic(&response, "error", response.target, 0, 0,
                                       response.message);
        result = nullptr;
        throw "compile_service compile_only loaded target";
      }
      const bool handled_by_vital_reload = (existing == master_ob || existing == simul_efun_ob);
      destruct_object(existing);
      remove_destructed_objects();
      if (handled_by_vital_reload) {
        current_object = master_ob;
        result = find_object2(response.target.c_str());
      } else {
        result = load_object(response.target.c_str(), 1);
      }
    } else {
      result = load_object(response.target.c_str(), 1);
    }
  } catch (...) {
    restore_context(&econ);
  }
  pop_runtime_error_sink();
  pop_context(&econ);
  current_object = previous_object;

  set_compile_service_diagnostics_collector(nullptr);
  response.diagnostics = diagnostics;
  response.runtime_errors = runtime_errors;
  response.ok = (result != nullptr);
  if (response.ok) {
    response.files_ok = 1;
  } else {
    response.files_failed = 1;
    if (response.reason.empty()) {
      finalize_failed_compile(&response, had_existing_object);
    }
  }
  return response;
}

nlohmann::json make_result_json(const CompileServiceResponse &file_response) {
  return nlohmann::json{{"file", file_response.target},
                        {"ok", file_response.ok},
                        {"phase", file_response.phase},
                        {"reason", file_response.reason},
                        {"message", file_response.message},
                        {"diagnostics", file_response.diagnostics},
                        {"runtime_errors", file_response.runtime_errors}};
}

void increment_summary_reason(nlohmann::json *summary, std::string_view reason) {
  if (reason == "syntax_error") {
    (*summary)["syntax_error_count"] = summary->value("syntax_error_count", 0) + 1;
  } else if (reason == "reload_loaded_object_failed") {
    (*summary)["reload_failed_count"] = summary->value("reload_failed_count", 0) + 1;
  } else if (reason == "runtime_error") {
    (*summary)["runtime_error_count"] = summary->value("runtime_error_count", 0) + 1;
  } else if (reason == "unsupported_target_kind") {
    (*summary)["unsupported_count"] = summary->value("unsupported_count", 0) + 1;
  } else {
    (*summary)["service_error_count"] = summary->value("service_error_count", 0) + 1;
  }
}

}  // namespace

CompileServiceResponse execute_compile_service_request(const CompileServiceRequest &request) {
  if (request.op == "dev_test") {
    return execute_dev_test_request(request);
  }

  auto normalized_target = normalize_lpc_target(request.target);
  auto fs_target = target_to_filesystem_path(normalized_target);

  if (has_extension(normalized_target, ".h")) {
    return unsupported_target_response(normalized_target, "header",
                                       "header files are not standalone compile targets");
  }

  std::error_code ec;
  if (std::filesystem::is_directory(fs_target, ec)) {
    CompileServiceResponse response;
    response.version = 1;
    response.kind = "directory";
    response.target = normalized_target;

    std::vector<std::filesystem::path> files;
    for (auto const &entry : std::filesystem::recursive_directory_iterator(fs_target, ec)) {
      if (ec) {
        break;
      }
      if (!entry.is_regular_file()) {
        continue;
      }
      if (entry.path().extension() == ".c") {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());

    for (auto const &file : files) {
      auto relative = std::filesystem::relative(file, std::filesystem::current_path(), ec);
      auto lpc_file = "/" + (ec ? file.generic_string() : relative.generic_string());
      auto file_response = compile_single_file(lpc_file, request.mode);
      response.results.push_back(make_result_json(file_response));
      response.files_total++;
      if (file_response.ok) {
        response.files_ok++;
      } else {
        response.files_failed++;
        increment_summary_reason(&response.summary, file_response.reason);
      }
    }
    response.ok = (response.files_failed == 0);
    response.summary["files_total"] = response.files_total;
    response.summary["files_ok"] = response.files_ok;
    response.summary["files_failed"] = response.files_failed;
    response.summary["syntax_error_count"] = response.summary.value("syntax_error_count", 0);
    response.summary["reload_failed_count"] = response.summary.value("reload_failed_count", 0);
    response.summary["runtime_error_count"] = response.summary.value("runtime_error_count", 0);
    response.summary["unsupported_count"] = response.summary.value("unsupported_count", 0);
    response.summary["service_error_count"] = response.summary.value("service_error_count", 0);
    if (!response.ok) {
      set_compile_service_failure(&response, "compile", "directory_compile_failed",
                                  std::to_string(response.files_failed) + " of " +
                                      std::to_string(response.files_total) +
                                      " file(s) failed");
      add_compile_service_diagnostic(&response, "error", response.target, 0, 0,
                                     response.message);
    }
    return response;
  }

  if (std::filesystem::exists(fs_target, ec) && !std::filesystem::is_regular_file(fs_target, ec)) {
    return unsupported_target_response(normalized_target, "file",
                                       "target is not a regular LPC source file");
  }
  if (has_extension(normalized_target, ".c") && !std::filesystem::exists(fs_target, ec)) {
    return target_not_found_response(normalized_target);
  }

  return compile_single_file(normalized_target, request.mode);
}

}  // namespace compile_service
