# lpccp Queued Concurrency Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让多个 `lpccp` 可以并发请求同一个运行中的 `driver`，同时保持 `lpccp` 命令行和最终 JSON 响应形状不变，并在服务端内部以 FIFO 串行执行请求，未开始执行的请求排队等待超过 5 秒时失败。

**Architecture:** 保留现有 `lpccp -> named pipe -> driver compile service -> execute_compile_service_request()` 总链路，但把 `compile_service` 从“接到连接立即执行”改成“两层结构”：接入层负责读取请求并创建内部任务，单独的 worker 线程负责串行消费任务并进入 VM 执行区。排队超时只发生在任务尚未开始执行时，超时失败继续复用现有 `compile` 的 `diagnostics` 通道和 `dev_test` 的 `error` 通道，不新增顶层字段。

**Tech Stack:** C++17, Windows named pipes, `std::thread`, `std::mutex`, `std::condition_variable`, nlohmann/json, GTest, 现有 FluffOS runtime compile service

---

## 文件结构

- Modify: `D:/code/fluffos/src/compile_service.cc`
  - 把 compile service 改成“接入层 + FIFO 队列 + 单 worker 执行”。
- Modify: `D:/code/fluffos/src/compile_service.h`
  - 若需要补充新的内部生命周期接口或测试可见辅助函数。
- Modify: `D:/code/fluffos/src/compile_service_protocol.h`
  - 如需要补充兼容旧响应形状的超时失败 helper，就放在共享协议头里；不得修改请求/响应 JSON 形状。
- Modify: `D:/code/fluffos/src/main_lpccp.cc`
  - 仅在必要时做更稳健的阻塞等待或连接错误处理；不得改变 CLI 和 JSON 输出形状。
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`
  - 增加并发接入、FIFO 顺序、排队超时、关闭路径与“开始执行后不再受 5 秒限制”的测试。
- Modify: `D:/code/fluffos/docs/cli/lpccp.md`
  - 补充“支持并发发起但 driver 内部串行排队”和“排队等待超时 5 秒”的行为说明。

---

## Chunk 1: 队列失败语义与测试骨架

### Task 1: 明确兼容旧响应形状的排队超时结果

**Files:**
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`
- Modify: `D:/code/fluffos/src/compile_service_protocol.h`

- [ ] **Step 1: 为 compile 请求写排队超时响应测试**

在 `test_compile_service.cc` 中新增一个纯协议级测试，构造 compile 超时失败响应并断言：
- `ok == false`
- `kind == "file"` 或 `kind == "directory"` 保持不变
- `diagnostics` 至少包含一条 `severity == "error"` 的记录
- 不新增新的顶层 JSON 字段

- [ ] **Step 2: 为 dev_test 请求写排队超时响应测试**

在 `test_compile_service.cc` 中新增协议级测试，构造 dev-test 超时失败响应并断言：
- `ok == false`
- `kind == "dev_test"`
- `error.type == "queue_timeout"`
- 顶层字段仍与现有 `CompileServiceResponse` 一致

- [ ] **Step 3: 运行测试，确认先失败**

Run: `cmake --build build --target lpc_tests`

Expected:
- 新增测试因 helper 缺失或断言不满足而失败

- [ ] **Step 4: 在协议层补最小 helper**

在 `compile_service_protocol.h` 中补最小 helper 或最小构造逻辑，专门生成：
- compile 排队超时失败响应
- dev_test 排队超时失败响应

要求：
- 不改 `CompileServiceRequest` JSON 字段
- 不改 `CompileServiceResponse` JSON 序列化形状
- compile 超时通过 `diagnostics` 表达
- dev_test 超时通过 `error` 表达

- [ ] **Step 5: 运行测试，确认协议级失败语义通过**

Run: `cmake --build build --target lpc_tests`

Expected:
- 新增超时响应测试 PASS

- [ ] **Step 6: Commit**

```bash
git add src/compile_service_protocol.h src/tests/test_compile_service.cc
git commit -m "test: define queue timeout response expectations"
```

---

## Chunk 2: compile_service 队列化改造

### Task 2: 为 compile_service 建立 FIFO 队列和单 worker

**Files:**
- Modify: `D:/code/fluffos/src/compile_service.cc`
- Modify: `D:/code/fluffos/src/compile_service.h`
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] **Step 1: 写服务端串行队列测试**

在 `test_compile_service.cc` 中增加服务端行为测试，覆盖：
- 两个请求都能被服务接受
- worker 每次只执行一个任务
- 第二个任务必须在第一个任务完成后才开始执行

测试中可以用一个受控的假执行器、阻塞点或受控 hook 来观测执行顺序，避免依赖真实 LPC 编译时间偶然性。

- [ ] **Step 2: 运行测试，确认先失败**

Run: `cmake --build build --target lpc_tests`

Expected:
- 新增串行队列测试 FAIL

- [ ] **Step 3: 在 compile_service.cc 中引入内部任务结构**

实现内部 `PendingRequest`，至少包含：
- 原始 `CompileServiceRequest`
- 请求类型需要的目标路径
- 入队时间
- 与客户端回写结果相关的同步状态
- “已完成/已开始执行”标记

- [ ] **Step 4: 引入共享队列与同步原语**

在 `compile_service.cc` 中加入最小并发基础设施：
- `std::mutex`
- `std::condition_variable`
- `std::deque<PendingRequest>`
- 单独的 worker thread

要求：
- 队列顺序固定为 FIFO
- 任意时刻只有 worker 进入真实执行区

- [ ] **Step 5: 重写服务循环为接入层**

把现有“读请求后直接执行”的逻辑改成：
- 读取客户端请求
- 创建 `PendingRequest`
- 入队
- 唤醒 worker
- 等待该任务完成或被判定超时
- 将最终 JSON 响应写回当前连接

不要改变每个连接只处理单次请求的模型。

- [ ] **Step 6: 实现单 worker 串行执行**

worker 线程负责：
- 从 FIFO 队列取任务
- 标记任务开始执行
- 调用现有 `execute_compile_service_request()`
- 保存最终结果
- 唤醒对应等待中的接入线程

- [ ] **Step 7: 运行测试，确认串行队列语义通过**

Run: `cmake --build build --target lpc_tests`

Expected:
- 新增串行化测试 PASS
- 既有 compile service 生命周期测试继续 PASS

- [ ] **Step 8: Commit**

```bash
git add src/compile_service.cc src/compile_service.h src/tests/test_compile_service.cc
git commit -m "feat: queue compile service requests in driver"
```

---

## Chunk 3: 5 秒排队等待超时

### Task 3: 只对“未开始执行”请求施加 5000ms 超时

**Files:**
- Modify: `D:/code/fluffos/src/compile_service.cc`
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] **Step 1: 写 compile 排队超时测试**

在 `test_compile_service.cc` 中新增测试：
- 第一个请求先占住 worker 超过 5 秒
- 第二个 compile 请求成功接入，但未开始执行
- 第二个请求最终返回 `ok == false`
- `diagnostics` 中包含排队超时信息

- [ ] **Step 2: 写 dev_test 排队超时测试**

新增测试：
- 第一个请求占住 worker 超过 5 秒
- 第二个 `dev_test` 请求未开始执行即超时
- 返回 `ok == false`
- `error.type == "queue_timeout"`

- [ ] **Step 3: 写“开始执行后不再受 5 秒限制”测试**

新增测试：
- 某请求在 5 秒内进入执行区
- 执行本身耗时超过 5 秒
- 最终仍按真实结果完成，而不是被超时中断

- [ ] **Step 4: 运行测试，确认先失败**

Run: `cmake --build build --target lpc_tests`

Expected:
- 新增超时相关测试 FAIL

- [ ] **Step 5: 在 worker 取任务时加入 5000ms 阈值判断**

实现逻辑：
- 超时判断点在“worker 即将开始执行任务”时
- 若当前时间减去任务入队时间大于 5000ms，则不调用 `execute_compile_service_request()`
- 直接构造兼容旧协议的超时失败响应
- 标记任务完成并唤醒接入线程

- [ ] **Step 6: 确保超时不影响已开始执行的任务**

实现时要保证：
- 只在任务未开始执行时检查 5000ms
- 一旦 `started == true`，即使运行更久，也不再进入超时分支

- [ ] **Step 7: 运行测试，确认超时语义通过**

Run: `cmake --build build --target lpc_tests`

Expected:
- compile/dev_test 排队超时测试 PASS
- “开始执行后不再受 5 秒限制”测试 PASS

- [ ] **Step 8: Commit**

```bash
git add src/compile_service.cc src/tests/test_compile_service.cc
git commit -m "feat: time out queued compile requests after 5 seconds"
```

---

## Chunk 4: 并发接入稳定性与客户端兼容

### Task 4: 确保多个 lpccp 并发发起时客户端行为保持不变

**Files:**
- Modify: `D:/code/fluffos/src/compile_service.cc`
- Modify: `D:/code/fluffos/src/main_lpccp.cc`
- Modify: `D:/code/fluffos/src/tests/test_compile_service.cc`

- [ ] **Step 1: 写并发客户端端到端测试**

在 `test_compile_service.cc` 中增加端到端测试，覆盖：
- 两个客户端近乎同时连接同一个 pipe
- 两个请求都能得到完整 JSON
- 客户端 stdout 的 JSON 形状与当前保持一致
- 成功请求退出码仍为 `0`
- 排队超时请求退出码仍为 `1`

- [ ] **Step 2: 运行测试，确认先失败**

Run: `cmake --build build --target lpccp lpc_tests`

Expected:
- 新增端到端并发测试 FAIL

- [ ] **Step 3: 只在必要时微调 main_lpccp.cc**

如果队列化后的服务端需要更稳健的客户端等待逻辑，则在 `main_lpccp.cc` 中做最小改动，例如：
- 处理服务端延迟返回时的读取稳定性
- 保证仍然只读取单个 JSON 响应并据 `ok` 设退出码

不得修改：
- CLI 参数
- stdout JSON 形状
- 0/1/2 退出码语义

- [ ] **Step 4: 确保服务端关闭时等待中的客户端不会挂死**

在 `compile_service.cc` 中补关闭路径：
- 停止接收新请求
- 队列中未开始执行的请求得到稳定失败
- 接入线程能退出

- [ ] **Step 5: 运行端到端测试**

Run: `cmake --build build --target driver lpccp lpc_tests`

Expected:
- 并发客户端端到端测试 PASS
- 关闭路径测试 PASS

- [ ] **Step 6: Commit**

```bash
git add src/compile_service.cc src/main_lpccp.cc src/tests/test_compile_service.cc
git commit -m "feat: keep lpccp stable under queued concurrency"
```

---

## Chunk 5: 文档与回归验证

### Task 5: 更新 CLI 文档并做回归验证

**Files:**
- Modify: `D:/code/fluffos/docs/cli/lpccp.md`
- Verify: `D:/code/fluffos/docs/superpowers/specs/2026-04-05-lpccp-queued-concurrency-design.md`

- [ ] **Step 1: 更新 lpccp CLI 文档**

在 `docs/cli/lpccp.md` 中补充：
- 多个 `lpccp` 可以并发发起请求
- 同一个 `driver` 内部仍串行执行 compile-service 请求
- 排队等待超过 5 秒时会失败
- 超时只作用于“尚未开始执行”的请求

- [ ] **Step 2: 运行完整相关构建**

Run: `cmake --build build --target driver lpccp lpc_tests`

Expected:
- `driver`、`lpccp`、`lpc_tests` 全部成功构建

- [ ] **Step 3: 运行测试**

Run: `ctest --test-dir build --output-on-failure -R lpc_tests`

Expected:
- `lpc_tests` 全部 PASS

- [ ] **Step 4: 手工验证并发场景**

在两个终端里或用脚本并发触发：
- 一个长请求先占住 worker
- 一个 compile 请求验证超时失败
- 一个 `dev_test` 请求验证超时失败
- 一个在 5 秒内进入执行区但运行超过 5 秒的请求验证不会被中断

- [ ] **Step 5: 核对实现与 spec 一致**

检查以下事实是否成立：
- CLI 不变
- 请求 JSON 不变
- 顶层响应形状不变
- 同一 `driver` 内部单 worker 串行执行
- 排队超时固定为 5 秒

- [ ] **Step 6: Commit**

```bash
git add docs/cli/lpccp.md docs/superpowers/plans/2026-04-05-lpccp-queued-concurrency.md
git commit -m "docs: describe lpccp queued concurrency behavior"
```
