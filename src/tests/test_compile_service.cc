#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#include "comm.h"
#include "compile_service.h"
#include "compile_service_client.h"
#include "compile_service_protocol.h"

namespace {
using compile_service::CompileServiceDiagnostic;
using compile_service::CompileServiceRequest;
using compile_service::CompileServiceResponse;
using compile_service::ParsedCompileServiceClientArgs;
using compile_service::build_compile_service_stub_response;
using compile_service::build_compile_queue_timeout_response;
using compile_service::build_dev_test_queue_timeout_response;
using compile_service::clear_compile_service_request_executor_for_testing;
using compile_service::dispatch_compile_service_request_for_testing;
using compile_service::format_compile_service_connect_error;
using compile_service::format_compile_service_usage;
using compile_service::make_compile_service_id;
using compile_service::make_compile_service_pipe_name;
using compile_service::normalize_compile_service_path;
using compile_service::parse_compile_service_client_args;
using compile_service::set_compile_service_request_executor_for_testing;

struct FakeExecutorState {
  std::mutex mutex;
  std::condition_variable cv_started;
  std::condition_variable cv_release;
  std::vector<std::string> started_targets;
  bool first_request_started = false;
  bool release_first_request = false;
  int sleep_after_start_ms = 0;
  int in_flight = 0;
  int max_in_flight = 0;
};

FakeExecutorState *g_fake_executor_state = nullptr;

struct CompileServiceTestGuard {
  CompileServiceTestGuard() {
    if (compile_service_running()) {
      stop_compile_service();
    }
    clear_compile_service_request_executor_for_testing();
    g_fake_executor_state = nullptr;
  }

  ~CompileServiceTestGuard() {
    if (compile_service_running()) {
      stop_compile_service();
    }
    clear_compile_service_request_executor_for_testing();
    g_fake_executor_state = nullptr;
  }
};

#ifdef _WIN32
struct PipeRoundTripResult {
  unsigned long win32_error = 0;
  std::optional<CompileServiceResponse> response;
};

PipeRoundTripResult send_pipe_request(std::string_view pipe_name, const CompileServiceRequest &request) {
  PipeRoundTripResult result;
  HANDLE pipe =
      CreateFileA(std::string(pipe_name).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) {
    result.win32_error = GetLastError();
    return result;
  }

  auto payload = nlohmann::json(request).dump() + "\n";
  DWORD bytes_written = 0;
  if (!WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &bytes_written, nullptr)) {
    result.win32_error = GetLastError();
    CloseHandle(pipe);
    return result;
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

  if (!response_text.empty()) {
    result.response = nlohmann::json::parse(response_text).get<CompileServiceResponse>();
  }
  return result;
}
#endif

CompileServiceResponse fake_serializing_executor(const CompileServiceRequest &request) {
  auto *state = g_fake_executor_state;
  EXPECT_NE(state, nullptr);

  {
    std::unique_lock<std::mutex> lock(state->mutex);
    state->in_flight++;
    state->max_in_flight = std::max(state->max_in_flight, state->in_flight);
    state->started_targets.push_back(request.target);
    if (request.target == "/queue/first.c") {
      state->first_request_started = true;
      state->cv_started.notify_all();
      state->cv_release.wait(lock, [&] { return state->release_first_request; });
    }
  }

  if (request.target == "/queue/slow-started.c" && state->sleep_after_start_ms > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(state->sleep_after_start_ms));
  }

  CompileServiceResponse response;
  response.version = 1;
  response.ok = true;
  response.kind = "file";
  response.target = request.target;

  {
    std::lock_guard<std::mutex> lock(state->mutex);
    state->in_flight--;
  }

  return response;
}

TEST(CompileServiceProtocol, NormalizesConfigPathBeforeHashing) {
  auto lhs = make_compile_service_id("lpc_example_http/./config.hell");
  auto rhs = make_compile_service_id("lpc_example_http/config.hell");
  EXPECT_EQ(lhs, rhs);

  auto normalized = normalize_compile_service_path("lpc_example_http/./config.hell");
  auto absolute = std::filesystem::absolute("lpc_example_http/config.hell").lexically_normal().generic_string();
#ifdef _WIN32
  std::transform(absolute.begin(), absolute.end(), absolute.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
#endif
  EXPECT_EQ(normalized, absolute);
}

TEST(CompileServiceProtocol, StableIdIgnoresWindowsCaseDifferences) {
  auto normalized = normalize_compile_service_path("lpc_example_http/config.hell");
  auto variant = normalized;
#ifdef _WIN32
  std::transform(variant.begin(), variant.end(), variant.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
#endif

  EXPECT_EQ(make_compile_service_id(normalized), make_compile_service_id(variant));
}

TEST(CompileServiceProtocol, PipeNameIsDerivedFromId) {
  auto id = make_compile_service_id("lpc_example_http/config.hell");
  auto pipe_name = make_compile_service_pipe_name(id);

  EXPECT_TRUE(pipe_name.rfind("\\\\.\\pipe\\fluffos-lpccp-", 0) == 0);
  EXPECT_NE(pipe_name.find(id), std::string::npos);
}

TEST(CompileServiceProtocol, RequestJsonRoundTrips) {
  CompileServiceRequest request;
  request.version = 1;
  request.op = "compile";
  request.target = "/adm/single/master.c";

  nlohmann::json j = request;
  EXPECT_EQ(j["version"], 1);
  EXPECT_EQ(j["op"], "compile");
  EXPECT_EQ(j["target"], "/adm/single/master.c");

  auto parsed = j.get<CompileServiceRequest>();
  EXPECT_EQ(parsed.version, request.version);
  EXPECT_EQ(parsed.op, request.op);
  EXPECT_EQ(parsed.target, request.target);
}

TEST(CompileServiceProtocol, DevTestRequestJsonRoundTrips) {
  CompileServiceRequest request;
  request.version = 1;
  request.op = "dev_test";
  request.target = "/single/tests/dev/dev_test_success.c";

  nlohmann::json j = request;
  EXPECT_EQ(j["version"], 1);
  EXPECT_EQ(j["op"], "dev_test");
  EXPECT_EQ(j["target"], "/single/tests/dev/dev_test_success.c");

  auto parsed = j.get<CompileServiceRequest>();
  EXPECT_EQ(parsed.version, request.version);
  EXPECT_EQ(parsed.op, request.op);
  EXPECT_EQ(parsed.target, request.target);
}

TEST(CompileServiceProtocol, ResponseJsonRoundTrips) {
  CompileServiceResponse response;
  response.version = 1;
  response.ok = false;
  response.kind = "file";
  response.target = "/adm/single/master.c";
  response.diagnostics.push_back(
      CompileServiceDiagnostic{"warning", "/adm/single/master.c", 12, 7, "Unused local variable"});

  nlohmann::json j = response;
  EXPECT_EQ(j["version"], 1);
  EXPECT_EQ(j["ok"], false);
  EXPECT_EQ(j["kind"], "file");
  EXPECT_EQ(j["target"], "/adm/single/master.c");
  EXPECT_EQ(j["diagnostics"].size(), 1);
  EXPECT_EQ(j["diagnostics"][0]["severity"], "warning");
  EXPECT_EQ(j["diagnostics"][0]["column"], 7);

  auto parsed = j.get<CompileServiceResponse>();
  EXPECT_EQ(parsed.version, response.version);
  EXPECT_EQ(parsed.ok, response.ok);
  EXPECT_EQ(parsed.kind, response.kind);
  EXPECT_EQ(parsed.target, response.target);
  ASSERT_EQ(parsed.diagnostics.size(), 1u);
  EXPECT_EQ(parsed.diagnostics[0].severity, "warning");
  EXPECT_EQ(parsed.diagnostics[0].file, "/adm/single/master.c");
  EXPECT_EQ(parsed.diagnostics[0].line, 12);
  EXPECT_EQ(parsed.diagnostics[0].column, 7);
  EXPECT_EQ(parsed.diagnostics[0].message, "Unused local variable");
}

TEST(CompileServiceProtocol, DevTestResponseJsonRoundTrips) {
  CompileServiceResponse response;
  response.version = 1;
  response.ok = true;
  response.kind = "dev_test";
  response.target = "/single/tests/dev/dev_test_success.c";
  response.output = {"setup complete", "running assertions"};
  response.result = nlohmann::json{
      {"ok", true},
      {"summary", "basic dev test passed"},
      {"checks", nlohmann::json::array({nlohmann::json{{"name", "sanity"}, {"ok", true}}})},
      {"artifacts", nlohmann::json::object()}};

  nlohmann::json j = response;
  EXPECT_EQ(j["kind"], "dev_test");
  EXPECT_EQ(j["output"].size(), 2);
  EXPECT_EQ(j["result"]["summary"], "basic dev test passed");

  auto parsed = j.get<CompileServiceResponse>();
  EXPECT_EQ(parsed.kind, response.kind);
  EXPECT_EQ(parsed.target, response.target);
  ASSERT_EQ(parsed.output.size(), 2u);
  EXPECT_EQ(parsed.output[0], "setup complete");
  EXPECT_EQ(parsed.result["ok"], true);
  EXPECT_EQ(parsed.result["checks"][0]["name"], "sanity");
}

TEST(CompileServiceProtocol, CompileQueueTimeoutUsesDiagnosticsWithoutNewTopLevelFields) {
  auto response = build_compile_queue_timeout_response("file", "/adm/single/master.c", 5000);
  nlohmann::json j = response;

  EXPECT_EQ(response.ok, false);
  EXPECT_EQ(response.kind, "file");
  EXPECT_EQ(response.target, "/adm/single/master.c");
  ASSERT_EQ(response.diagnostics.size(), 1u);
  EXPECT_EQ(response.diagnostics[0].severity, "error");
  EXPECT_EQ(response.diagnostics[0].line, 0);
  EXPECT_NE(response.diagnostics[0].message.find("5000ms"), std::string::npos);

  EXPECT_FALSE(j.contains("queue_state"));
  EXPECT_FALSE(j.contains("queue_timeout_ms"));
  EXPECT_FALSE(j.contains("request_id"));
  EXPECT_FALSE(j.contains("error"));
}

TEST(CompileServiceProtocol, DevTestQueueTimeoutUsesExistingErrorField) {
  auto response = build_dev_test_queue_timeout_response("/single/tests/dev/dev_test_success.c", 5000);
  nlohmann::json j = response;

  EXPECT_EQ(response.ok, false);
  EXPECT_EQ(response.kind, "dev_test");
  EXPECT_EQ(response.target, "/single/tests/dev/dev_test_success.c");
  EXPECT_TRUE(response.diagnostics.empty());
  ASSERT_TRUE(response.error.is_object());
  EXPECT_EQ(response.error["type"], "queue_timeout");
  std::string message = response.error["message"];
  EXPECT_NE(message.find("5000ms"), std::string::npos);

  EXPECT_FALSE(j.contains("queue_state"));
  EXPECT_FALSE(j.contains("queue_timeout_ms"));
  EXPECT_FALSE(j.contains("request_id"));
}

TEST(CompileServiceClient, ParsesConfigPathArguments) {
  ParsedCompileServiceClientArgs args;
  std::vector<std::string_view> argv = {"lpc_example_http/config.hell", "/adm/single/master.c"};

  ASSERT_TRUE(parse_compile_service_client_args(argv, &args));
  EXPECT_EQ(args.op, "compile");
  EXPECT_FALSE(args.use_explicit_id);
  EXPECT_EQ(args.config_or_id, "lpc_example_http/config.hell");
  EXPECT_EQ(args.target, "/adm/single/master.c");
}

TEST(CompileServiceClient, ParsesExplicitIdArguments) {
  ParsedCompileServiceClientArgs args;
  std::vector<std::string_view> argv = {"--id", "abc123", "/adm/single/"};

  ASSERT_TRUE(parse_compile_service_client_args(argv, &args));
  EXPECT_EQ(args.op, "compile");
  EXPECT_TRUE(args.use_explicit_id);
  EXPECT_EQ(args.config_or_id, "abc123");
  EXPECT_EQ(args.target, "/adm/single/");
}

TEST(CompileServiceClient, ParsesDevTestArguments) {
  ParsedCompileServiceClientArgs args;
  std::vector<std::string_view> argv = {"--dev-test", "lpc_example_http/config.hell",
                                        "/single/tests/dev/dev_test_success.c"};

  ASSERT_TRUE(parse_compile_service_client_args(argv, &args));
  EXPECT_EQ(args.op, "dev_test");
  EXPECT_FALSE(args.use_explicit_id);
  EXPECT_EQ(args.config_or_id, "lpc_example_http/config.hell");
  EXPECT_EQ(args.target, "/single/tests/dev/dev_test_success.c");
}

TEST(CompileServiceClient, ParsesDevTestExplicitIdArguments) {
  ParsedCompileServiceClientArgs args;
  std::vector<std::string_view> argv = {"--dev-test", "--id", "abc123",
                                        "/single/tests/dev/dev_test_success.c"};

  ASSERT_TRUE(parse_compile_service_client_args(argv, &args));
  EXPECT_EQ(args.op, "dev_test");
  EXPECT_TRUE(args.use_explicit_id);
  EXPECT_EQ(args.config_or_id, "abc123");
  EXPECT_EQ(args.target, "/single/tests/dev/dev_test_success.c");
}

TEST(CompileServiceClient, RejectsInvalidArgumentShapes) {
  ParsedCompileServiceClientArgs args;
  EXPECT_FALSE(parse_compile_service_client_args({}, &args));
  EXPECT_FALSE(parse_compile_service_client_args({"only-one"}, &args));
  EXPECT_FALSE(parse_compile_service_client_args({"--id", "missing-target"}, &args));
  EXPECT_FALSE(parse_compile_service_client_args({"--dev-test", "missing-target"}, &args));
  EXPECT_FALSE(parse_compile_service_client_args({"--dev-test", "--id", "missing-target"}, &args));
}

TEST(CompileServiceClient, FormatsUsageAndConnectErrors) {
  EXPECT_NE(format_compile_service_usage().find("lpccp <config-path> <path>"), std::string::npos);
  EXPECT_NE(format_compile_service_usage().find("lpccp --dev-test <config-path> <path>"),
            std::string::npos);
  auto message = format_compile_service_connect_error("\\\\.\\pipe\\fluffos-lpccp-test", 2);
  EXPECT_NE(message.find("fluffos-lpccp-test"), std::string::npos);
  EXPECT_NE(message.find("win32=2"), std::string::npos);
}

TEST(CompileServiceClient, StubResponseIncludesDerivedFields) {
  ParsedCompileServiceClientArgs args;
  args.config_or_id = "lpc_example_http/config.hell";
  args.target = "/adm/single/master.c";

  auto id = make_compile_service_id(args.config_or_id);
  auto response = build_compile_service_stub_response(args, id);

  EXPECT_EQ(response["id"], id);
  EXPECT_EQ(response["pipe_name"], make_compile_service_pipe_name(id));
  EXPECT_EQ(response["target"], "/adm/single/master.c");
  EXPECT_EQ(response["kind"], "client_stub");
  EXPECT_EQ(response["diagnostics"].size(), 1);
}

TEST(CompileServiceLifecycle, StopResetsRunningState) {
  ASSERT_FALSE(compile_service_running());
  ASSERT_TRUE(start_compile_service("testsuite/etc/config.test"));
  ASSERT_TRUE(compile_service_running());
  stop_compile_service();
  EXPECT_FALSE(compile_service_running());
}

TEST(CompileServiceLifecycle, RuntimeOutputSinkIsThreadLocal) {
  push_runtime_output_sink([](std::string_view) {});
  ASSERT_TRUE(has_runtime_output_sink());

  bool other_thread_has_sink = true;
  std::thread worker([&other_thread_has_sink] { other_thread_has_sink = has_runtime_output_sink(); });
  worker.join();

  pop_runtime_output_sink();
  EXPECT_FALSE(other_thread_has_sink);
  EXPECT_FALSE(has_runtime_output_sink());
}

TEST(CompileServiceLifecycle, QueuedRequestsRunOneAtATimeInFifoOrder) {
  CompileServiceTestGuard guard;
  FakeExecutorState state;
  g_fake_executor_state = &state;
  set_compile_service_request_executor_for_testing(&fake_serializing_executor);

  ASSERT_FALSE(compile_service_running());
  ASSERT_TRUE(start_compile_service("testsuite/etc/config.test"));

  CompileServiceRequest first_request;
  first_request.op = "compile";
  first_request.target = "/queue/first.c";

  CompileServiceRequest second_request;
  second_request.op = "compile";
  second_request.target = "/queue/second.c";

  std::optional<CompileServiceResponse> first_response;
  std::optional<CompileServiceResponse> second_response;

  std::thread first_thread(
      [&] { first_response = dispatch_compile_service_request_for_testing(first_request); });

  {
    std::unique_lock<std::mutex> lock(state.mutex);
    ASSERT_TRUE(state.cv_started.wait_for(lock, std::chrono::seconds(1),
                                          [&] { return state.first_request_started; }));
  }

  std::thread second_thread(
      [&] { second_response = dispatch_compile_service_request_for_testing(second_request); });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    ASSERT_EQ(state.started_targets.size(), 1u);
    EXPECT_EQ(state.started_targets[0], "/queue/first.c");
    EXPECT_EQ(state.max_in_flight, 1);
  }

  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.release_first_request = true;
  }
  state.cv_release.notify_all();

  first_thread.join();
  second_thread.join();

  ASSERT_TRUE(first_response.has_value());
  ASSERT_TRUE(second_response.has_value());
  EXPECT_TRUE(first_response->ok);
  EXPECT_TRUE(second_response->ok);

  {
    std::lock_guard<std::mutex> lock(state.mutex);
    ASSERT_EQ(state.started_targets.size(), 2u);
    EXPECT_EQ(state.started_targets[0], "/queue/first.c");
    EXPECT_EQ(state.started_targets[1], "/queue/second.c");
    EXPECT_EQ(state.max_in_flight, 1);
  }
}

TEST(CompileServiceLifecycle, CompileRequestsTimeOutAfterWaitingFiveSecondsInQueue) {
  CompileServiceTestGuard guard;
  FakeExecutorState state;
  g_fake_executor_state = &state;
  set_compile_service_request_executor_for_testing(&fake_serializing_executor);

  ASSERT_FALSE(compile_service_running());
  ASSERT_TRUE(start_compile_service("testsuite/etc/config.test"));

  CompileServiceRequest first_request;
  first_request.op = "compile";
  first_request.target = "/queue/first.c";

  CompileServiceRequest second_request;
  second_request.op = "compile";
  second_request.target = "/queue/second.c";

  std::optional<CompileServiceResponse> first_response;
  std::optional<CompileServiceResponse> second_response;

  std::thread first_thread(
      [&] { first_response = dispatch_compile_service_request_for_testing(first_request); });

  {
    std::unique_lock<std::mutex> lock(state.mutex);
    ASSERT_TRUE(state.cv_started.wait_for(lock, std::chrono::seconds(1),
                                          [&] { return state.first_request_started; }));
  }

  std::thread second_thread(
      [&] { second_response = dispatch_compile_service_request_for_testing(second_request); });

  std::this_thread::sleep_for(std::chrono::milliseconds(5200));

  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.release_first_request = true;
  }
  state.cv_release.notify_all();

  first_thread.join();
  second_thread.join();

  ASSERT_TRUE(first_response.has_value());
  ASSERT_TRUE(second_response.has_value());
  EXPECT_TRUE(first_response->ok);
  EXPECT_FALSE(second_response->ok);
  EXPECT_EQ(second_response->kind, "file");
  ASSERT_EQ(second_response->diagnostics.size(), 1u);
  EXPECT_EQ(second_response->diagnostics[0].severity, "error");
  EXPECT_NE(second_response->diagnostics[0].message.find("5000ms"), std::string::npos);
}

TEST(CompileServiceLifecycle, DevTestRequestsTimeOutAfterWaitingFiveSecondsInQueue) {
  CompileServiceTestGuard guard;
  FakeExecutorState state;
  g_fake_executor_state = &state;
  set_compile_service_request_executor_for_testing(&fake_serializing_executor);

  ASSERT_FALSE(compile_service_running());
  ASSERT_TRUE(start_compile_service("testsuite/etc/config.test"));

  CompileServiceRequest first_request;
  first_request.op = "compile";
  first_request.target = "/queue/first.c";

  CompileServiceRequest second_request;
  second_request.op = "dev_test";
  second_request.target = "/queue/dev-test.c";

  std::optional<CompileServiceResponse> first_response;
  std::optional<CompileServiceResponse> second_response;

  std::thread first_thread(
      [&] { first_response = dispatch_compile_service_request_for_testing(first_request); });

  {
    std::unique_lock<std::mutex> lock(state.mutex);
    ASSERT_TRUE(state.cv_started.wait_for(lock, std::chrono::seconds(1),
                                          [&] { return state.first_request_started; }));
  }

  std::thread second_thread(
      [&] { second_response = dispatch_compile_service_request_for_testing(second_request); });

  std::this_thread::sleep_for(std::chrono::milliseconds(5200));

  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.release_first_request = true;
  }
  state.cv_release.notify_all();

  first_thread.join();
  second_thread.join();

  ASSERT_TRUE(first_response.has_value());
  ASSERT_TRUE(second_response.has_value());
  EXPECT_TRUE(first_response->ok);
  EXPECT_FALSE(second_response->ok);
  EXPECT_EQ(second_response->kind, "dev_test");
  ASSERT_TRUE(second_response->error.is_object());
  EXPECT_EQ(second_response->error["type"], "queue_timeout");
}

TEST(CompileServiceLifecycle, RequestsThatStartWithinFiveSecondsCanRunLongerThanFiveSeconds) {
  CompileServiceTestGuard guard;
  FakeExecutorState state;
  state.sleep_after_start_ms = 5200;
  g_fake_executor_state = &state;
  set_compile_service_request_executor_for_testing(&fake_serializing_executor);

  ASSERT_FALSE(compile_service_running());
  ASSERT_TRUE(start_compile_service("testsuite/etc/config.test"));

  CompileServiceRequest request;
  request.op = "compile";
  request.target = "/queue/slow-started.c";

  auto response = dispatch_compile_service_request_for_testing(request);

  EXPECT_TRUE(response.ok);
  EXPECT_EQ(response.kind, "file");
  EXPECT_EQ(response.target, "/queue/slow-started.c");
}

#ifdef _WIN32
TEST(CompileServiceTransport, ConcurrentPipeClientsCanBothReceiveResponses) {
  CompileServiceTestGuard guard;
  FakeExecutorState state;
  g_fake_executor_state = &state;
  set_compile_service_request_executor_for_testing(&fake_serializing_executor);

  ASSERT_TRUE(start_compile_service("testsuite/etc/config.test"));
  auto pipe_name = make_compile_service_pipe_name(compile_service_id());

  CompileServiceRequest first_request;
  first_request.op = "compile";
  first_request.target = "/queue/first.c";

  CompileServiceRequest second_request;
  second_request.op = "compile";
  second_request.target = "/queue/second.c";

  std::optional<PipeRoundTripResult> first_result;
  std::optional<PipeRoundTripResult> second_result;

  std::thread first_thread([&] { first_result = send_pipe_request(pipe_name, first_request); });

  bool first_started = false;
  {
    std::unique_lock<std::mutex> lock(state.mutex);
    first_started = state.cv_started.wait_for(lock, std::chrono::seconds(1),
                                             [&] { return state.first_request_started; });
  }

  if (!first_started) {
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      state.release_first_request = true;
    }
    state.cv_release.notify_all();
    first_thread.join();
    FAIL() << "pipe transport did not route requests through the queued executor";
  }

  std::thread second_thread([&] { second_result = send_pipe_request(pipe_name, second_request); });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  {
    std::lock_guard<std::mutex> lock(state.mutex);
    ASSERT_EQ(state.started_targets.size(), 1u);
    EXPECT_EQ(state.started_targets[0], "/queue/first.c");
  }

  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.release_first_request = true;
  }
  state.cv_release.notify_all();

  first_thread.join();
  second_thread.join();

  ASSERT_TRUE(first_result.has_value());
  ASSERT_TRUE(second_result.has_value());
  EXPECT_EQ(first_result->win32_error, 0u);
  EXPECT_EQ(second_result->win32_error, 0u);
  ASSERT_TRUE(first_result->response.has_value());
  ASSERT_TRUE(second_result->response.has_value());
  EXPECT_TRUE(first_result->response->ok);
  EXPECT_TRUE(second_result->response->ok);
}
#endif
}  // namespace
