# Dev Test Runner Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a minimal runtime development tool that invokes a target object's zero-argument `dev_test()` entrypoint, captures development output, and returns structured JSON over the existing `lpccp` / compile_service channel.

**Architecture:** Reuse the existing Windows `lpccp` named-pipe transport and extend the compile service protocol with a second operation, `dev_test`. Implement runtime execution in a dedicated runner that loads or finds the target object, safely calls `dev_test()`, captures request-scoped output emitted through the normal driver message path, validates the returned JSON string, and packages success or failure into a structured response.

**Tech Stack:** C++17 driver code, existing compile_service named pipe transport, `nlohmann::json`, existing LPC runtime (`safe_apply`, `load_object`, `find_object2`), GTest `lpc_tests`, Windows `build.cmd`.

---

## File Structure

**Existing files to modify**

- `src/compile_service_protocol.h`
  Responsibility: Extend request/response schema to support `dev_test` without breaking compile requests.
- `src/compile_service_client.h`
  Responsibility: Parse the new CLI flag shape and produce a consistent request object.
- `src/main_lpccp.cc`
  Responsibility: Populate the request `op`, send it over the pipe, and print the JSON response exactly once.
- `src/compile_service.cc`
  Responsibility: Continue to deserialize requests and dispatch by operation.
- `src/runtime_compile_request.h`
  Responsibility: Expose a single dispatch function for runtime compile/dev-test execution, or hand off to a new dev-test runner.
- `src/runtime_compile_request.cc`
  Responsibility: Keep compile behavior unchanged and route `op == "dev_test"` to the new dev-test runner.
- `src/comm.h`
  Responsibility: Declare a scoped output-capture hook used by the dev-test request.
- `src/comm.cc`
  Responsibility: Implement request-scoped output capture in the message path so `write` / `printf` output can be collected without building a second logging channel.
- `src/tests/test_compile_service.cc`
  Responsibility: Add protocol/client parsing coverage for the new operation.
- `src/tests/test_lpc.cc`
  Responsibility: Add runtime tests for `dev_test` execution, output capture, JSON success, and runtime error handling.

**New files to create**

- `src/runtime_dev_test_request.h`
  Responsibility: Declare the runtime `dev_test` execution entrypoint and any minimal helper types local to the feature.
- `src/runtime_dev_test_request.cc`
  Responsibility: Implement object resolution, safe invocation of `dev_test`, output capture lifecycle, return-value validation, and structured error responses.
- `testsuite/single/tests/dev/dev_test_success.c`
  Responsibility: Minimal LPC fixture that prints progress and returns a valid JSON string.
- `testsuite/single/tests/dev/dev_test_runtime_error.c`
  Responsibility: Minimal LPC fixture that prints progress then throws a runtime error.
- `testsuite/single/tests/dev/dev_test_bad_return.c`
  Responsibility: Minimal LPC fixture that returns a non-string or invalid JSON string to exercise validation failures.

**Why this split**

- Keep compile logic and dev-test logic separate. Compile behavior is already sensitive and recently fixed around vital object reload; mixing feature-specific behavior into the same function will make future debugging harder.
- Put output capture in `comm.cc` instead of inventing a new ad hoc printer path. `write()` and `printf()` already end up in the driver message pipeline, so the least invasive place to collect them is where the bytes are already centralized.

---

## Behavior Contract

The finished tool should support:

- `lpccp <config-path> <path>` for compile requests exactly as today.
- `lpccp --dev-test <config-path> <path>` for dev-test requests.
- `lpccp --dev-test --id <id> <path>` only if the existing explicit-id form is already supported for the chosen parse shape.

The runtime contract for `dev_test` is:

- The target object is identified by LPC path.
- If already loaded, use the current loaded object.
- If not loaded, load it once.
- Do not provide arbitrary function dispatch; only call `dev_test`.
- Call it with zero arguments.
- Expect a return value of type `string`.
- Parse that string as JSON.
- Return structured tool JSON containing:
  - request success/failure
  - captured output lines
  - parsed result object on success
  - structured error block on failure

Suggested wire response shape:

```json
{
  "version": 1,
  "kind": "dev_test",
  "target": "/single/tests/dev/dev_test_success.c",
  "ok": true,
  "output": [
    "setup complete",
    "running assertions"
  ],
  "result": {
    "ok": true,
    "summary": "basic dev test passed",
    "checks": [
      { "name": "sanity", "ok": true }
    ],
    "artifacts": {}
  }
}
```

Suggested failure shape:

```json
{
  "version": 1,
  "kind": "dev_test",
  "target": "/single/tests/dev/dev_test_runtime_error.c",
  "ok": false,
  "output": [
    "before failure"
  ],
  "error": {
    "type": "runtime_error",
    "message": "Error text from LPC"
  }
}
```

---

## Chunk 1: Protocol And CLI

### Task 1: Extend protocol data structures

**Files:**
- Modify: `src/compile_service_protocol.h`
- Test: `src/tests/test_compile_service.cc`

- [ ] **Step 1: Add failing protocol tests for `dev_test` request/response round-trips**

Add tests that prove:
- request JSON can carry `"op": "dev_test"`
- response JSON can carry `"kind": "dev_test"`
- response JSON can carry `"output"` and `"result"` or `"error"`

- [ ] **Step 2: Run only the protocol tests and verify they fail for missing fields**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=CompileServiceProtocol.*`

Expected: new `dev_test` protocol cases fail because the structs do not yet support the new fields.

- [ ] **Step 3: Extend `CompileServiceRequest` and `CompileServiceResponse` minimally**

Add only the fields needed for this feature. Recommended additions:

```cpp
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
  std::vector<std::string> output;
  nlohmann::json result;
  nlohmann::json error;
};
```

Do not remove existing compile fields even if they are unused by `dev_test`.

- [ ] **Step 4: Update `to_json` / `from_json` to include the new optional fields**

Keep compile compatibility:
- compile responses should still round-trip
- `dev_test` responses should omit unused fields cleanly

- [ ] **Step 5: Run the protocol tests again**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=CompileServiceProtocol.*`

Expected: all protocol tests pass.

- [ ] **Step 6: Commit**

```bash
git add src/compile_service_protocol.h src/tests/test_compile_service.cc
git commit -m "feat: extend compile service protocol for dev test"
```

### Task 2: Add CLI shape for `--dev-test`

**Files:**
- Modify: `src/compile_service_client.h`
- Modify: `src/main_lpccp.cc`
- Test: `src/tests/test_compile_service.cc`

- [ ] **Step 1: Add failing client parse tests**

Cover:
- `lpccp --dev-test <config> <path>`
- optionally `lpccp --dev-test --id <id> <path>` if supported
- invalid mixed forms are rejected

- [ ] **Step 2: Run only the client tests and verify they fail**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=CompileServiceClient.*`

Expected: the new parse tests fail.

- [ ] **Step 3: Extend `ParsedCompileServiceClientArgs` with request op**

Recommended:

```cpp
struct ParsedCompileServiceClientArgs {
  bool use_explicit_id = false;
  std::string config_or_id;
  std::string target;
  std::string op = "compile";
};
```

- [ ] **Step 4: Update `parse_compile_service_client_args` and usage text**

Keep the old form valid.
Add a new accepted form:

```text
lpccp --dev-test <config-path> <path>
```

- [ ] **Step 5: Update `main_lpccp.cc` to send `request.op = args.op`**

Do not add any client-side interpretation of LPC return values. The client still only prints the returned JSON.

- [ ] **Step 6: Run client/protocol tests**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=CompileServiceClient.*:CompileServiceProtocol.*`

Expected: pass.

- [ ] **Step 7: Commit**

```bash
git add src/compile_service_client.h src/main_lpccp.cc src/tests/test_compile_service.cc
git commit -m "feat: add lpccp dev-test request mode"
```

---

## Chunk 2: Runtime Dev Test Execution

### Task 3: Add a dedicated runtime dev-test runner

**Files:**
- Create: `src/runtime_dev_test_request.h`
- Create: `src/runtime_dev_test_request.cc`
- Modify: `src/runtime_compile_request.h`
- Modify: `src/runtime_compile_request.cc`
- Test: `src/tests/test_lpc.cc`

- [ ] **Step 1: Add failing `DriverTest` coverage for a successful dev-test request**

Write a test that:
- constructs a `CompileServiceRequest` with `op = "dev_test"`
- targets a new LPC fixture object
- asserts `response.ok == true`
- asserts `response.kind == "dev_test"`
- asserts `response.result["ok"] == true`

- [ ] **Step 2: Run that one test and verify failure**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=DriverTest.*DevTest*`

Expected: fail because the runtime cannot handle `dev_test`.

- [ ] **Step 3: Create `runtime_dev_test_request.h/.cc`**

Implement a function with a small, focused API, e.g.:

```cpp
namespace compile_service {
CompileServiceResponse execute_dev_test_request(const CompileServiceRequest &request);
}
```

Responsibilities inside:
- normalize target path
- resolve object with `find_object2` or `load_object`
- set `current_object` to the target object while calling
- clear `command_giver` so `write()` falls through the noninteractive message path
- invoke `safe_apply("dev_test", target_ob, 0, ORIGIN_DRIVER)`
- convert failures into structured `response.error`

- [ ] **Step 4: Route requests by op in `runtime_compile_request.cc`**

Recommended structure:

```cpp
if (request.op == "dev_test") {
  return execute_dev_test_request(request);
}
return execute_compile_request(request);
```

Do not merge compile and dev-test execution into one long function.

- [ ] **Step 5: Validate return type**

Inside the dev-test runner:
- if object cannot be loaded/found, return `error.type = "object_unavailable"`
- if `dev_test` is missing, return `error.type = "missing_entrypoint"`
- if return type is not string, return `error.type = "bad_return_type"`
- if JSON parse of the returned string fails, return `error.type = "invalid_json"`

- [ ] **Step 6: Parse the returned string into `response.result`**

Use `nlohmann::json::parse` on the returned LPC string. The tool response should carry parsed JSON, not only the raw string.

- [ ] **Step 7: Run the new success-path test**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=DriverTest.*DevTest*`

Expected: success-path test passes, output-related assertions still fail until capture is added.

- [ ] **Step 8: Commit**

```bash
git add src/runtime_dev_test_request.h src/runtime_dev_test_request.cc src/runtime_compile_request.h src/runtime_compile_request.cc src/tests/test_lpc.cc
git commit -m "feat: add runtime dev-test request execution"
```

---

## Chunk 3: Output Capture

### Task 4: Add request-scoped output capture for `write` / `printf`

**Files:**
- Modify: `src/comm.h`
- Modify: `src/comm.cc`
- Modify: `src/runtime_dev_test_request.cc`
- Test: `src/tests/test_lpc.cc`

- [ ] **Step 1: Add a failing test that asserts printed output is returned**

The LPC fixture should call `write("setup complete\n")` and `printf("value=%d\n", 1)`.
The test should assert `response.output` contains those lines.

- [ ] **Step 2: Run the focused test and verify it fails**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=DriverTest.*DevTest*`

Expected: `response.output` is empty.

- [ ] **Step 3: Add a small scoped capture API in `comm.h`**

Recommended API:

```cpp
using runtime_output_sink_t = std::function<void(std::string_view)>;
void push_runtime_output_sink(runtime_output_sink_t sink);
void pop_runtime_output_sink();
```

Keep this API private to the driver, not exposed as a public protocol feature.

- [ ] **Step 4: Implement scoped capture in `comm.cc`**

Hook `add_message()` so that when:
- there is an active runtime sink
- and the message is going through the noninteractive path (`who == nullptr` or no valid interactive target)

the sink receives the message text before normal fallback behavior.

Important:
- keep normal stderr/log behavior unless you explicitly decide to suppress duplication for dev-test requests
- do not globally redirect all output forever
- make the capture stack-safe and exception-safe

- [ ] **Step 5: In `runtime_dev_test_request.cc`, install the sink around only the `safe_apply("dev_test", ...)` call**

Collect output into `std::vector<std::string>` or a line-buffered accumulator.

Recommended simple behavior:
- append raw chunks during execution
- split into lines at the end for `response.output`

- [ ] **Step 6: Re-run the dev-test success-path tests**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=DriverTest.*DevTest*`

Expected: output assertions pass.

- [ ] **Step 7: Commit**

```bash
git add src/comm.h src/comm.cc src/runtime_dev_test_request.cc src/tests/test_lpc.cc
git commit -m "feat: capture dev-test runtime output"
```

---

## Chunk 4: Failure Handling

### Task 5: Add runtime-error and invalid-return coverage

**Files:**
- Create: `testsuite/single/tests/dev/dev_test_runtime_error.c`
- Create: `testsuite/single/tests/dev/dev_test_bad_return.c`
- Modify: `src/tests/test_lpc.cc`

- [ ] **Step 1: Add failing tests for runtime errors**

Cover:
- `dev_test` throws runtime error after printing one line
- response returns `ok == false`
- response has `error.type == "runtime_error"`
- captured output still includes the line printed before failure

- [ ] **Step 2: Add failing tests for bad return values**

Cover:
- missing `dev_test`
- non-string return
- invalid JSON string return

- [ ] **Step 3: Run the focused failure-path suite and verify red**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=DriverTest.*DevTest*`

Expected: failure-path tests fail.

- [ ] **Step 4: Implement structured error mapping in `runtime_dev_test_request.cc`**

Suggested error types:
- `object_unavailable`
- `missing_entrypoint`
- `runtime_error`
- `bad_return_type`
- `invalid_json`

Include human-readable `message` in each.

- [ ] **Step 5: Re-run the focused failure-path suite**

Run: `D:\code\fluffos\build\work\src\tests\lpc_tests.exe --gtest_filter=DriverTest.*DevTest*`

Expected: all dev-test tests pass.

- [ ] **Step 6: Commit**

```bash
git add testsuite/single/tests/dev/dev_test_runtime_error.c testsuite/single/tests/dev/dev_test_bad_return.c src/tests/test_lpc.cc src/runtime_dev_test_request.cc
git commit -m "test: cover dev-test runtime failures"
```

---

## Chunk 5: End-To-End Verification

### Task 6: Add transport-level tests and manual verification notes

**Files:**
- Modify: `src/tests/test_compile_service.cc`
- Modify: `docs/cli/lpccp.md` if the CLI docs already exist and need the new mode documented

- [ ] **Step 1: Add client parse coverage for `--dev-test` if not already done**

Keep this test close to the existing compile-service client tests.

- [ ] **Step 2: Build and run the full unit suite**

Run:

```bash
cmake --build D:\code\fluffos\build\work --target lpc_tests -j 8
D:\code\fluffos\build\work\src\tests\lpc_tests.exe
```

Expected: all tests pass.

- [ ] **Step 3: Rebuild Windows artifacts**

Run:

```bash
D:\code\fluffos\build.cmd
```

Expected:
- `build/dist/driver.exe`
- `build/dist/lpccp.exe`
- current installer `.exe`

- [ ] **Step 4: Manual end-to-end verification**

Run the driver:

```bash
D:\code\fluffos\build\dist\driver.exe D:\code\fluffos\testsuite\etc\config.test
```

In another shell, run:

```bash
D:\code\fluffos\build\dist\lpccp.exe --dev-test D:\code\fluffos\testsuite\etc\config.test /single/tests/dev/dev_test_success.c
```

Expected:
- `kind == "dev_test"`
- `ok == true`
- `output` contains printed progress lines
- `result.ok == true`

Then run:

```bash
D:\code\fluffos\build\dist\lpccp.exe --dev-test D:\code\fluffos\testsuite\etc\config.test /single/tests/dev/dev_test_runtime_error.c
```

Expected:
- `kind == "dev_test"`
- `ok == false`
- `output` contains the last printed line before failure
- `error.type == "runtime_error"`

- [ ] **Step 5: Commit**

```bash
git add src/tests/test_compile_service.cc docs/cli/lpccp.md
git commit -m "docs: describe lpccp dev-test mode"
```

---

## Notes For The Implementer

- Do not add arbitrary function invocation.
- Do not add external parameters.
- Do not build an object-handle/session system.
- Keep the execution entrypoint hardcoded to `dev_test`.
- Preserve existing compile-service behavior exactly.
- Prefer returning structured errors over throwing transport-level failures once the request has reached the driver.
- Keep output capture narrowly scoped to the request to avoid cross-request leaks on the compile-service thread.
- Because compile service runs on a dedicated thread, any global capture mechanism must be carefully scoped and stack-disciplined even if only one request is currently processed at a time.

## Risks To Watch

- `write()` currently falls into the normal message path. If the capture hook is placed too late or too early, output may be lost or doubled.
- `current_object` / `command_giver` state must be restored after `dev_test`, even on error.
- Do not destruct/reload objects for `dev_test` unless the final design explicitly requires it; this tool is for runtime debugging, so reusing the live loaded object is the least surprising default.
- If `dev_test` targets an interactive object, be careful not to leak output to a real player session by accident.

## Verification Checklist

- `lpccp --dev-test` parses correctly.
- Compile mode still works unchanged.
- `dev_test` can load or find target object.
- `dev_test` output is captured and returned.
- Runtime errors become structured `error` responses.
- Invalid JSON returns become structured `invalid_json` responses.
- No `client_stub` regressions.
- Windows `build.cmd` still produces `dist`, install image, and installer.

## Suggested Follow-Up After This Plan

Once the minimal runner works, the next incremental feature should be a mudlib convention doc for `dev_test` payload shape:
- `ok`
- `summary`
- `checks`
- `artifacts`

Do that separately from the runner implementation so the runner stays generic and small.

