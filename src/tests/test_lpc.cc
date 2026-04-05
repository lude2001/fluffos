#include <gtest/gtest.h>
#include "base/package_api.h"

#include "mainlib.h"

#include "compile_service_protocol.h"
#include "runtime_compile_request.h"
#include "compiler/internal/compiler.h"
#include "vm/internal/apply.h"
#include "vm/internal/simulate.h"

namespace {
size_t count_loaded_objects_named(const char *name) {
  size_t count = 0;
  for (auto *ob = obj_list; ob != nullptr; ob = ob->next_all) {
    if (ob->obname && strcmp(ob->obname, name) == 0) {
      count++;
    }
  }
  return count;
}
}

// Test fixture class
class DriverTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    chdir(TESTSUITE_DIR);
    // Initialize libevent, This should be done before executing LPC.
    auto* base = init_main("etc/config.test");
    vm_start();
  }

 protected:
  void SetUp() override { clear_state(); }

  void TearDown() override { clear_state(); }
};

TEST_F(DriverTest, TestCompileDumpProgWorks) {
  current_object = master_ob;
  const char* file = "single/master.c";
  struct object_t* obj = nullptr;

  error_context_t econ{};
  save_context(&econ);
  try {
    obj = find_object(file);
  } catch (...) {
    restore_context(&econ);
    FAIL();
  }
  pop_context(&econ);

  ASSERT_NE(obj, nullptr);
  ASSERT_NE(obj->prog, nullptr);

  dump_prog(obj->prog, stdout, 1 | 2);
}

TEST_F(DriverTest, TestInMemoryCompileFile) {
  program_t* prog = nullptr;

  std::istringstream source("void test() {}");
  auto stream = std::make_unique<IStreamLexStream>(source);
  prog = compile_file(std::move(stream), "test");

  ASSERT_NE(prog, nullptr);
  deallocate_program(prog);
}

TEST_F(DriverTest, TestInMemoryCompileFileFail) {
  program_t* prog = nullptr;
  std::istringstream source("aksdljfaljdfiasejfaeslfjsaef");
  auto stream = std::make_unique<IStreamLexStream>(source);
  prog = compile_file(std::move(stream), "test");

  ASSERT_EQ(prog, nullptr);
}

TEST_F(DriverTest, TestInMemoryCompileUnusedLocalWarningUsesDeclarationLocation) {
  std::vector<compile_service::CompileServiceDiagnostic> diagnostics;
  set_compile_service_diagnostics_collector(&diagnostics);

  std::istringstream source("void test() {\n    int cc = 1;\n}\n");
  auto stream = std::make_unique<IStreamLexStream>(source);
  auto *prog = compile_file(std::move(stream), "test");

  set_compile_service_diagnostics_collector(nullptr);

  ASSERT_NE(prog, nullptr);
  ASSERT_EQ(diagnostics.size(), 1u);
  EXPECT_EQ(diagnostics[0].severity, "warning");
  EXPECT_EQ(diagnostics[0].line, 2);
  EXPECT_EQ(diagnostics[0].column, 9);
  EXPECT_EQ(diagnostics[0].message, "Unused local variable 'cc'");

  deallocate_program(prog);
}

TEST_F(DriverTest, TestCompileServiceMasterRecompileDoesNotDuplicateMasterObject) {
  auto before = count_loaded_objects_named("single/master");
  ASSERT_EQ(before, 1u);
  ASSERT_EQ(master_ob, find_object2("/single/master.c"));

  compile_service::CompileServiceRequest request;
  request.target = "/single/master.c";
  auto response = compile_service::execute_compile_service_request(request);

  ASSERT_TRUE(response.ok);
  EXPECT_EQ(count_loaded_objects_named("single/master"), 1u);
  EXPECT_EQ(master_ob, find_object2("/single/master.c"));
}

TEST_F(DriverTest, TestDevTestRequestReturnsStructuredJsonResult) {
  compile_service::CompileServiceRequest request;
  request.op = "dev_test";
  request.target = "/single/tests/dev/dev_test_success.c";

  auto response = compile_service::execute_compile_service_request(request);

  ASSERT_TRUE(response.ok);
  EXPECT_EQ(response.kind, "dev_test");
  ASSERT_GE(response.output.size(), 2u);
  EXPECT_EQ(response.output[0], "setup complete");
  EXPECT_EQ(response.result["ok"], 1);
  EXPECT_EQ(response.result["summary"], "basic dev test passed");
  ASSERT_EQ(response.result["checks"].size(), 1);
  EXPECT_EQ(response.result["checks"][0]["name"], "sanity");
}

TEST_F(DriverTest, TestDevTestRequestCapturesRuntimeError) {
  compile_service::CompileServiceRequest request;
  request.op = "dev_test";
  request.target = "/single/tests/dev/dev_test_runtime_error.c";

  auto response = compile_service::execute_compile_service_request(request);

  ASSERT_FALSE(response.ok);
  EXPECT_EQ(response.kind, "dev_test");
  ASSERT_FALSE(response.error.is_null());
  EXPECT_EQ(response.error["type"], "runtime_error");
  EXPECT_EQ(response.error["message"], "dev_test exploded\n");
  ASSERT_GE(response.output.size(), 1u);
  EXPECT_EQ(response.output[0], "before failure");
}

TEST_F(DriverTest, TestDevTestRequestRejectsBadReturnType) {
  compile_service::CompileServiceRequest request;
  request.op = "dev_test";
  request.target = "/single/tests/dev/dev_test_bad_return.c";

  auto response = compile_service::execute_compile_service_request(request);

  ASSERT_FALSE(response.ok);
  EXPECT_EQ(response.kind, "dev_test");
  ASSERT_FALSE(response.error.is_null());
  EXPECT_EQ(response.error["type"], "bad_return_type");
  ASSERT_GE(response.output.size(), 1u);
  EXPECT_EQ(response.output[0], "returning bad payload");
}

TEST_F(DriverTest, TestExplicitInheritedCallSkipsPrototypePlaceholder) {
  std::vector<compile_service::CompileServiceDiagnostic> diagnostics;
  set_compile_service_diagnostics_collector(&diagnostics);

  auto *obj = load_object("/single/tests/compiler/inherit_proto_child", 1);

  set_compile_service_diagnostics_collector(nullptr);

  ASSERT_NE(obj, nullptr);
  for (const auto &diagnostic : diagnostics) {
    EXPECT_EQ(diagnostic.message.find("BUG: inherit function is undefined or prototype"),
              std::string::npos);
  }

  auto *result = safe_apply("call_parent", obj, 0, ORIGIN_DRIVER);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->type, T_STRING);
  EXPECT_STREQ(result->u.string, "ok");
}

TEST_F(DriverTest, TestNamedInheritedCallThroughPrototypePlaceholder) {
  std::vector<compile_service::CompileServiceDiagnostic> diagnostics;
  set_compile_service_diagnostics_collector(&diagnostics);

  auto *obj = load_object("/clone/user/inherit_named_call", 1);

  set_compile_service_diagnostics_collector(nullptr);

  ASSERT_NE(obj, nullptr);
  for (const auto &diagnostic : diagnostics) {
    EXPECT_EQ(diagnostic.message.find("BUG: inherit function is undefined or prototype"),
              std::string::npos);
  }

  auto *result = safe_apply("test_call", obj, 0, ORIGIN_DRIVER);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->type, T_STRING);
  EXPECT_STREQ(result->u.string, "foobar");
}

TEST_F(DriverTest, TestValidLPC_FunctionDeafultArgument) {
  const char* source = R"(
// default case
void test1() {
}

// default case
void test2(int a, int b) {
  ASSERT_EQ(a, 1);
  ASSERT_EQ(b, 2);
}

// varargs
void test3(int a, int* b ...) {
  ASSERT_EQ(a, 1);
  ASSERT_EQ(b[0], 2);
  ASSERT_EQ(b[1], 3);
  ASSERT_EQ(b[2], 4);
  ASSERT_EQ(b[3], 5);
}

// can have multiple trailing arguments with a FP for calculating default value
void test4(int a, string b: (: "str" :), int c: (: 0 :)) {
  switch(a) {
    case 1: {
      ASSERT_EQ("str", b);
      ASSERT_EQ(0, c);
      break;
    }
    case 2: {
      ASSERT_EQ("aaa", b);
      ASSERT_EQ(0, c);
      break;
    }
    case 3: {
      ASSERT_EQ("bbb", b);
      ASSERT_EQ(3, c);
      break;
    }
  }
}

void do_tests() {
    test1();
    test2(1, 2);
    test3(1, 2, 3, 4, 5);
    // direct call
    test4(1);
    test4(2, "aaa");
    test4(3, "bbb", 3);
    // apply
    this_object()->test4(1);
    this_object()->test4(2, "aaa");
    this_object()->test4(3, "bbb", 3);
}
  )";
  std::istringstream iss(source);
  auto stream = std::make_unique<IStreamLexStream>(iss);
  auto *prog = compile_file(std::move(stream), "test");

  ASSERT_NE(prog, nullptr);
  dump_prog(prog, stdout, 1 | 2);
  deallocate_program(prog);
}


TEST_F(DriverTest, TestLPC_FunctionInherit) {
    // Load the inherited object first
    error_context_t econ{};
    save_context(&econ);
    try {
    auto obj = find_object("/single/tests/compiler/function");
    ASSERT_NE(obj , nullptr);

    auto obj2 = find_object("/single/tests/compiler/function_inherit");
    ASSERT_NE(obj2 , nullptr);

    auto obj3 = find_object("/single/tests/compiler/function_inherit_2");
    ASSERT_NE(obj3 , nullptr);

    dump_prog(obj3->prog, stdout, 1 | 2);
    } catch (...) {
        restore_context(&econ);
        FAIL();
    }
    pop_context(&econ);

}
