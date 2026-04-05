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

void set_error(CompileServiceResponse *response, std::string_view type, std::string message) {
  response->ok = false;
  response->error = nlohmann::json{{"type", type}, {"message", std::move(message)}};
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

  push_runtime_output_sink(
      [&captured_output](std::string_view chunk) { captured_output.append(chunk); });
  struct SinkGuard {
    ~SinkGuard() {
      pop_runtime_output_sink();
      pop_runtime_error_sink();
    }
  } sink_guard;
  push_runtime_error_sink(
      [&captured_error](std::string_view chunk) { captured_error.assign(chunk); });

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

  if (!target_ob) {
    set_error(&response, "object_unavailable", current_error_message());
    return response;
  }

  if (!function_exists("dev_test", target_ob, 0)) {
    set_error(&response, "missing_entrypoint", "Object does not define dev_test()");
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

  if (!ret) {
    set_error(&response, "runtime_error",
              captured_error.empty() ? current_error_message() : captured_error);
    return response;
  }

  if (ret->type != T_STRING) {
    set_error(&response, "bad_return_type", "dev_test() must return a JSON string");
    return response;
  }

  try {
    response.result = nlohmann::json::parse(ret->u.string);
  } catch (const nlohmann::json::exception &e) {
    set_error(&response, "invalid_json", e.what());
    return response;
  }

  response.ok = true;
  return response;
}

}  // namespace compile_service
