#include "base/std.h"
#include "runtime_dev_test_request.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "comm.h"
#include "vm/internal/apply.h"
#include "vm/internal/base/interpret.h"
#include "vm/internal/base/machine.h"
#include "vm/internal/base/object.h"
#include "vm/internal/base/program.h"
#include "vm/internal/master.h"
#include "vm/internal/simulate.h"

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

std::vector<std::string> split_output_lines(const std::string &buffer) {
  std::vector<std::string> lines;
  std::string current;

  for (char ch : buffer) {
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      lines.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(ch);
  }

  if (!current.empty()) {
    lines.push_back(current);
  }

  return lines;
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

void set_error(CompileServiceResponse *response,
               std::string_view phase,
               std::string_view reason,
               std::string message,
               std::string_view legacy_error_type = {}) {
  set_compile_service_failure(response, phase, reason, message);
  response->error = nlohmann::json{{"type", legacy_error_type.empty() ? reason : legacy_error_type},
                                   {"message", response->message}};
}

}  // namespace

CompileServiceResponse execute_dev_test_request(const CompileServiceRequest &request) {
  CompileServiceResponse response;
  response.version = 1;
  response.kind = "dev_test";
  response.target = normalize_lpc_target(request.target);

  object_t *previous_object = current_object;
  std::string captured_output;
  std::string captured_error;
  std::vector<nlohmann::json> runtime_errors;

  push_runtime_output_sink(
      [&captured_output](std::string_view chunk) { captured_output.append(chunk); });
  struct SinkGuard {
    ~SinkGuard() {
      pop_runtime_output_sink();
      pop_runtime_error_sink();
    }
  } sink_guard;
  push_runtime_error_sink([&](std::string_view chunk) {
    captured_error.assign(chunk);
    runtime_errors.push_back(make_runtime_error(chunk));
  });

  object_t *target_ob = nullptr;
  error_context_t econ{};
  save_context(&econ);
  try {
    current_object = master_ob;
    target_ob = find_object2(response.target.c_str());
    if (!target_ob) {
      target_ob = load_object(response.target.c_str(), 1);
    }
  } catch (...) {
    restore_context(&econ);
  }
  pop_context(&econ);
  current_object = previous_object;
  response.output = split_output_lines(captured_output);
  response.runtime_errors = runtime_errors;

  if (!target_ob) {
    response.compile_status = "failed";
    response.test_status = "not_run";
    set_error(&response, "compile", "compile_failed", current_error_message(), "object_unavailable");
    return response;
  }

  response.compile_status = "ok";
  if (!function_exists("dev_test", target_ob, 0)) {
    response.test_status = "missing";
    response.test_message = "Object does not define dev_test()";
    set_error(&response, "dev_test", "test_missing", response.test_message, "missing_entrypoint");
    return response;
  }

  svalue_t *ret = nullptr;
  save_context(&econ);
  save_command_giver(nullptr);
  try {
    ret = apply("dev_test", target_ob, 0, ORIGIN_DRIVER);
  } catch (...) {
    restore_context(&econ);
  }
  restore_command_giver();
  pop_context(&econ);
  current_object = previous_object;
  response.output = split_output_lines(captured_output);
  response.runtime_errors = runtime_errors;

  if (!ret) {
    response.test_status = "failed";
    response.test_message = captured_error.empty() ? current_error_message() : captured_error;
    set_error(&response, "runtime", "runtime_error", response.test_message);
    return response;
  }

  if (ret->type != T_STRING) {
    response.test_status = "failed";
    response.test_message = "dev_test() must return a JSON string";
    set_error(&response, "dev_test", "test_failed", response.test_message, "bad_return_type");
    return response;
  }

  try {
    response.result = nlohmann::json::parse(ret->u.string);
  } catch (const nlohmann::json::exception &e) {
    response.test_status = "failed";
    response.test_message = e.what();
    set_error(&response, "dev_test", "test_failed", response.test_message, "invalid_json");
    return response;
  }

  response.ok = true;
  response.test_status = "ok";
  return response;
}

}  // namespace compile_service
