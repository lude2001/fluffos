#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <thread>
#include <nlohmann/json.hpp>

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
using compile_service::format_compile_service_connect_error;
using compile_service::format_compile_service_usage;
using compile_service::make_compile_service_id;
using compile_service::make_compile_service_pipe_name;
using compile_service::normalize_compile_service_path;
using compile_service::parse_compile_service_client_args;

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
}  // namespace
