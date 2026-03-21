#include <gtest/gtest.h>

#include <filesystem>
#include <nlohmann/json.hpp>

#include "compile_service_protocol.h"

namespace {
using compile_service::CompileServiceDiagnostic;
using compile_service::CompileServiceRequest;
using compile_service::CompileServiceResponse;
using compile_service::make_compile_service_id;
using compile_service::make_compile_service_pipe_name;
using compile_service::normalize_compile_service_path;

TEST(CompileServiceProtocol, NormalizesConfigPathBeforeHashing) {
  auto lhs = make_compile_service_id("lpc_example_http/./config.hell");
  auto rhs = make_compile_service_id("lpc_example_http/config.hell");
  EXPECT_EQ(lhs, rhs);

  auto normalized = normalize_compile_service_path("lpc_example_http/./config.hell");
  auto absolute = std::filesystem::absolute("lpc_example_http/config.hell").lexically_normal().generic_string();
  EXPECT_EQ(normalized, absolute);
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

TEST(CompileServiceProtocol, ResponseJsonRoundTrips) {
  CompileServiceResponse response;
  response.version = 1;
  response.ok = false;
  response.kind = "file";
  response.target = "/adm/single/master.c";
  response.diagnostics.push_back(
      CompileServiceDiagnostic{"warning", "/adm/single/master.c", 12, "Unused local variable"});

  nlohmann::json j = response;
  EXPECT_EQ(j["version"], 1);
  EXPECT_EQ(j["ok"], false);
  EXPECT_EQ(j["kind"], "file");
  EXPECT_EQ(j["target"], "/adm/single/master.c");
  EXPECT_EQ(j["diagnostics"].size(), 1);
  EXPECT_EQ(j["diagnostics"][0]["severity"], "warning");

  auto parsed = j.get<CompileServiceResponse>();
  EXPECT_EQ(parsed.version, response.version);
  EXPECT_EQ(parsed.ok, response.ok);
  EXPECT_EQ(parsed.kind, response.kind);
  EXPECT_EQ(parsed.target, response.target);
  ASSERT_EQ(parsed.diagnostics.size(), 1u);
  EXPECT_EQ(parsed.diagnostics[0].severity, "warning");
  EXPECT_EQ(parsed.diagnostics[0].file, "/adm/single/master.c");
  EXPECT_EQ(parsed.diagnostics[0].line, 12);
  EXPECT_EQ(parsed.diagnostics[0].message, "Unused local variable");
}
}  // namespace
