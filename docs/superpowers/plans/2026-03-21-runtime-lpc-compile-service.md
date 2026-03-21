# Runtime LPC Compile Service Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为运行中的 FluffOS driver 增加一个本地 IPC 编译服务，并提供 `lpccp <config-path> <path>` 客户端，让本地工具可以请求单文件或目录级真实重编译并获得 JSON 诊断结果。

**Architecture:** `driver` 在完成 config 加载和 VM 启动后创建一个基于 config 身份的 named pipe 服务端；每个编译请求都在当前 VM 内按真实 LPC 语义执行“销毁旧对象 -> 重新 load_object() -> 收集 smart_log() 诊断”。`lpccp` 是一个轻量客户端，只负责连 pipe、发 JSON 请求、收 JSON 响应并据此设置退出码。

**Tech Stack:** C++17, Windows named pipes, nlohmann/json, 现有 FluffOS VM/编译器链路, GTest

---

## 文件结构

- 新建 `D:/code/fluffos/src/compile_service_protocol.h`
  - 共享请求/响应结构、`id` 生成规则、pipe 名称生成规则、config 路径规范化规则。
- 新建 `D:/code/fluffos/src/compile_service.h`
  - 暴露 driver 侧服务生命周期接口。
- 新建 `D:/code/fluffos/src/compile_service.cc`
  - 实现 named pipe 监听、请求解析、串行调度与 JSON 响应输出。
- 新建 `D:/code/fluffos/src/runtime_compile_request.h`
  - 暴露单文件与目录编译执行接口。
- 新建 `D:/code/fluffos/src/runtime_compile_request.cc`
  - 实现“强制销毁 + 真实重编译 + 目录聚合”逻辑。
- 新建 `D:/code/fluffos/src/main_lpccp.cc`
  - 实现 `lpccp <config-path> <path>` 客户端入口，并保留 `--id` 调试入口。
- 修改 `D:/code/fluffos/src/mainlib.cc`
  - 在 `driver_main()` 中挂接 compile service 的启动与关闭。
- 修改 `D:/code/fluffos/src/CMakeLists.txt`
  - 把新模块加入构建，并新增 `lpccp` 可执行程序与安装规则。
- 修改 `D:/code/fluffos/src/compiler/internal/compiler.h`
  - 暴露请求级 diagnostics collector 接口。
- 修改 `D:/code/fluffos/src/compiler/internal/compiler.cc`
  - 在 `smart_log()` 中附加写入结构化 diagnostics。
- 修改 `D:/code/fluffos/src/tests/CMakeLists.txt`
  - 把新测试文件加入 `lpc_tests`。
- 新建 `D:/code/fluffos/src/tests/test_compile_service.cc`
  - 覆盖 `id`/pipe 名称、diagnostics 收集、单文件重编译、目录聚合等测试。

---

## Chunk 1: 协议与构建骨架

### Task 1: 建立共享协议头和客户端入口

**Files:**
- Create: `D:/code/fluffos/src/compile_service_protocol.h`
- Create: `D:/code/fluffos/src/main_lpccp.cc`
- Modify: `D:/code/fluffos/src/CMakeLists.txt`
- Test: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] 新增协议级测试用例，先只断言 config 路径规范化、`id` 生成、pipe 名称生成、请求 JSON 编解码接口的期望形状。
- [ ] 运行 `cmake --build build --target lpc_tests`，确认新增测试因为符号缺失或文件缺失而失败。
- [ ] 在 `compile_service_protocol.h` 中定义共享结构：请求类型、响应类型、诊断记录、`make_compile_service_id()`、`make_compile_service_pipe_name()`。
- [ ] 在 `main_lpccp.cc` 中写出最小客户端骨架：参数解析、config 路径转 `id`、pipe 名称解析、标准输出打印响应文本、标准错误打印连接错误。
- [ ] 在 `src/CMakeLists.txt` 中新增 `lpccp` 目标和安装规则。
- [ ] 重新运行 `cmake --build build --target lpccp lpc_tests`，确认协议级测试通过、`lpccp` 能完成编译。
- [ ] 提交本任务，提交信息使用 `feat: add lpccp protocol scaffolding`。

### Task 2: 为计划中的测试骨架准备 GTest 入口

**Files:**
- Modify: `D:/code/fluffos/src/tests/CMakeLists.txt`
- Create: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] 新增 `test_compile_service.cc` 并建立独立测试 fixture，复用 `init_main("etc/config.test")` 与 `vm_start()`。
- [ ] 先加入最小测试集，只覆盖协议 helper 和空 diagnostics collector。
- [ ] 运行 `cmake --build build --target lpc_tests`。
- [ ] 运行 `ctest --test-dir build --output-on-failure -R lpc_tests`，确认测试被发现并执行。
- [ ] 提交本任务，提交信息使用 `test: add compile service test skeleton`。

---

## Chunk 2: 结构化诊断收集

### Task 3: 给编译器增加请求级 diagnostics collector

**Files:**
- Modify: `D:/code/fluffos/src/compiler/internal/compiler.h`
- Modify: `D:/code/fluffos/src/compiler/internal/compiler.cc`
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] 在测试里先写出 collector 行为断言：collector 激活时 `smart_log()` 追加一条结构化记录，collector 未激活时不追加记录。
- [ ] 运行 `cmake --build build --target lpc_tests`，确认测试先失败。
- [ ] 在 `compiler.h`/`compiler.cc` 中增加请求级 collector API，例如设置当前 collector、清理当前 collector、读取当前 collector。
- [ ] 修改 `smart_log()`：保留现有 `debug_message` 和 `safe_apply_master_ob(APPLY_LOG_ERROR, 2)`，同时在 collector 存在时追加结构化记录。
- [ ] 给 diagnostics 结构固定字段：`severity`、`file`、`line`、`message`。
- [ ] 重新运行 `cmake --build build --target lpc_tests`。
- [ ] 运行 `ctest --test-dir build --output-on-failure -R lpc_tests`，确认 collector 测试通过。
- [ ] 提交本任务，提交信息使用 `feat: collect compiler diagnostics for IPC requests`。

---

## Chunk 3: 运行时重编译执行器

### Task 4: 实现单文件真实重编译辅助逻辑

**Files:**
- Create: `D:/code/fluffos/src/runtime_compile_request.h`
- Create: `D:/code/fluffos/src/runtime_compile_request.cc`
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] 在测试中先写出单文件执行器的行为：请求一个合法 LPC 文件时返回 `ok=true`；请求一个非法文件时返回 `ok=false` 且包含 diagnostics。
- [ ] 在测试中增加“已加载对象会被强制重新编译”的断言，避免误用 `reload_object()` 语义。
- [ ] 运行 `cmake --build build --target lpc_tests`，确认测试先失败。
- [ ] 在 `runtime_compile_request.cc` 中实现单文件流程：路径规范化、`find_object2()`、`destruct_object()`、`remove_destructed_objects()`、`load_object()`、结果 JSON 化。
- [ ] 在同一模块中封装请求级 collector 的启停，确保每个单文件请求只拿到自己的 diagnostics。
- [ ] 重新运行 `cmake --build build --target lpc_tests`。
- [ ] 运行 `ctest --test-dir build --output-on-failure -R lpc_tests`，确认单文件测试通过。
- [ ] 提交本任务，提交信息使用 `feat: add runtime single-file recompilation helper`。

### Task 5: 实现目录递归编译与聚合结果

**Files:**
- Modify: `D:/code/fluffos/src/runtime_compile_request.h`
- Modify: `D:/code/fluffos/src/runtime_compile_request.cc`
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] 在测试中先写出目录请求断言：递归找到 `.c` 文件、按字典序执行、单个文件失败时目录结果仍然完整返回。
- [ ] 运行 `cmake --build build --target lpc_tests`，确认目录测试先失败。
- [ ] 在执行器中加入目录遍历、`.c` 过滤、排序、逐文件执行和结果聚合。
- [ ] 固定目录返回字段：`kind`、`files_total`、`files_ok`、`files_failed`、`results`。
- [ ] 重新运行 `cmake --build build --target lpc_tests`。
- [ ] 运行 `ctest --test-dir build --output-on-failure -R lpc_tests`，确认目录聚合测试通过。
- [ ] 提交本任务，提交信息使用 `feat: add directory runtime compilation aggregation`。

---

## Chunk 4: driver 服务端与 lpccp 端到端链路

### Task 6: 在 driver 中挂接 named pipe 编译服务

**Files:**
- Create: `D:/code/fluffos/src/compile_service.h`
- Create: `D:/code/fluffos/src/compile_service.cc`
- Modify: `D:/code/fluffos/src/mainlib.cc`
- Modify: `D:/code/fluffos/src/CMakeLists.txt`
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] 在测试里先写服务端请求调度测试：一个 JSON 请求能被解析为单文件或目录执行，多个请求按串行顺序处理。
- [ ] 运行 `cmake --build build --target lpc_tests`，确认服务端测试先失败。
- [ ] 在 `compile_service.cc` 中实现服务生命周期：启动、停止、单请求读取、请求分发、JSON 响应写回。
- [ ] 在 `mainlib.cc` 的 `driver_main()` 中，在 `vm_start()` 之后、`backend()` 之前启动服务；在退出路径上清理服务。
- [ ] 让服务端使用 config 文件路径生成稳定 `id` 和 pipe 名称。
- [ ] 重新运行 `cmake --build build --target driver lpc_tests`。
- [ ] 运行 `ctest --test-dir build --output-on-failure -R lpc_tests`，确认服务调度测试通过。
- [ ] 提交本任务，提交信息使用 `feat: add driver-owned compile IPC service`。

### Task 7: 完成 lpccp 客户端并验证端到端行为

**Files:**
- Modify: `D:/code/fluffos/src/main_lpccp.cc`
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] 在测试中先写客户端端到端断言：客户端能接收 config 路径、内部计算 `id`、连接 pipe、发送单文件请求、打印返回 JSON、按 `ok` 状态设置退出码。
- [ ] 运行 `cmake --build build --target lpccp lpc_tests`，确认端到端测试先失败。
- [ ] 完成 `main_lpccp.cc` 的 Windows pipe 连接与单请求收发逻辑。
- [ ] 明确退出码规则：`0` 表示全部成功，`1` 表示存在编译失败，`2` 表示 IPC 或请求级异常。
- [ ] 重新运行 `cmake --build build --target driver lpccp lpc_tests`。
- [ ] 运行 `ctest --test-dir build --output-on-failure -R lpc_tests`，确认端到端测试通过。
- [ ] 手工验证：
  - 启动 driver：`build/src/driver.exe lpc_example_http/config.hell`
  - 单文件请求：`build/src/lpccp.exe lpc_example_http/config.hell /adm/single/master.c`
  - 目录请求：`build/src/lpccp.exe lpc_example_http/config.hell /adm/single/`
- [ ] 提交本任务，提交信息使用 `feat: add lpccp runtime compile client`。

---

## 收尾验证

### Task 8: 做最终回归验证并更新文档说明

**Files:**
- Modify: `D:/code/fluffos/docs/cli/index.md`
- Create or Modify: `D:/code/fluffos/docs/cli/lpccp.md`
- Verify: `D:/code/fluffos/docs/superpowers/specs/2026-03-21-runtime-lpc-compile-service-design.md`

- [ ] 补充 `lpccp` CLI 文档，覆盖 config 路径入口、可选 `--id` 调试入口、文件编译、目录编译、退出码和 JSON 输出。
- [ ] 运行 `cmake --build build --target driver lpccp lpc_tests`。
- [ ] 运行 `ctest --test-dir build --output-on-failure -R lpc_tests`。
- [ ] 手工验证一组成功请求和一组语法错误请求，确认 JSON 中 `diagnostics` 字段完整。
- [ ] 检查 spec 与实现是否一致，必要时回填设计文档中的实现偏差。
- [ ] 提交本任务，提交信息使用 `docs: document lpccp runtime compile service`。
