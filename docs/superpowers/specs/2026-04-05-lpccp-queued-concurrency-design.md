# lpccp 并发发起与服务端串行排队设计

**目标：** 在不修改 `lpccp` 命令行调用方式和最终 JSON 响应形状的前提下，让多个 `lpccp` 可以并发请求同一个运行中的 `driver`，并由 `driver` 在内部对真实编译 / `dev_test` 执行进行串行排队。

**背景：**
- 当前 `lpccp` 已经通过本地 named pipe 连接运行中的 `driver`，请求运行时真实重编译或 `dev_test()`。
- 现有实现是“建立连接后立即在服务线程内执行请求”，因此它本质上只适合单请求串行处理。
- FluffOS 当前这条编译与执行链路会读写共享 VM 状态，例如 `current_object`、对象析构与重载、运行时错误上下文以及请求级 diagnostics collector；这些状态不适合在同一个 `driver` 进程内并发执行。
- 现在缺的不是“并行编译能力”，而是“允许多个客户端并发发起，并由服务端可靠排队”的能力。

**核心决策：**
- `lpccp` 的命令行接口保持不变：
  - `lpccp <config-path> <path>`
  - `lpccp --dev-test <config-path> <path>`
  - `lpccp --id <id> <path>`
  - `lpccp --dev-test --id <id> <path>`
- 请求 JSON 负载保持不变。
- 成功与失败时的最终 JSON 响应形状保持不变；不新增顶层协议字段。
- 同一个运行中的 `driver` 只允许一个 compile-service worker 真正进入 VM 执行区。
- 多个 `lpccp` 可以同时发起请求；服务端负责将这些请求排入 FIFO 队列。
- 队列等待超时固定为 5 秒；该超时只作用于“尚未开始执行”的请求，不作用于已经开始执行的请求。

**非目标：**
- 不在同一个 `driver` 进程内实现真正的并行编译。
- 不修改 `lpccp` 的 stdout JSON 顶层字段。
- 不新增新的 CLI 参数来控制排队行为。
- 第一版不做同目标请求去重、合并或覆盖旧请求。
- 第一版不做多级优先队列。

**为什么不做进程内并行执行：**
- 当前运行时编译与 `dev_test` 都依赖共享 VM 状态，且实现不是可重入设计。
- 编译执行会触碰对象生命周期，包括 `find_object2()`、`destruct_object()`、`remove_destructed_objects()`、`load_object()`。
- 诊断收集依赖进程内共享 collector 指针。
- 因此，在同一个运行中的 `driver` 内做并行执行不是本设计的目标；排队串行是更符合 FluffOS 当前架构的方案。

**外部行为约束：**
- 对调用方来说，`lpccp` 仍然表现为“一次请求，一次响应”。
- 多个 `lpccp` 可同时启动并请求同一个 `driver`。
- 若请求在 5 秒内进入执行区，则行为与当前实现一致。
- 若请求在队列中等待超过 5 秒仍未开始执行，则返回现有失败通道下的 JSON，而不是新增协议形状。
- 一旦请求开始执行，即使总耗时超过 5 秒，也必须执行到结束。

**并发模型：**
- 服务端拆分为两个职责层：
  - 接入层：负责监听 pipe、接收客户端连接、读取现有请求 JSON、创建内部任务对象。
  - 执行层：只有一个 worker，从 FIFO 队列中取出任务并调用现有 `execute_compile_service_request()`。
- 队列中的每个任务至少包含：
  - 原始 `CompileServiceRequest`
  - 接收时间戳
  - 与客户端连接对应的 pipe handle 或等价回写通道
  - 同步原语，用于让接入线程等待最终执行结果
- worker 从队列头取任务时：
  - 若该任务等待时间未超过 5 秒，则执行真实请求。
  - 若已超过 5 秒，则不进入 VM 执行区，而是直接构造超时失败响应并回写给客户端。
- 队列顺序固定为 FIFO。

**超时语义：**
- 固定等待超时：5000ms。
- 超时起点：请求被接入并成功入队的时刻。
- 超时终点：worker 准备开始执行该请求的时刻。
- 超时仅代表“未在 5 秒内轮到执行”，不代表执行失败或执行超时。
- 对已经开始执行的请求，不再施加额外时间限制。

**失败语义与响应兼容：**
- 普通 `compile` 请求若因排队超时失败：
  - 返回现有 `CompileServiceResponse` 顶层结构。
  - `ok` 为 `false`。
  - `kind`、`target` 保持原语义。
  - 使用现有 `diagnostics` 通道表达失败原因。
  - 建议写入一条结构化 diagnostic：
    - `severity: "error"`
    - `file`: 若可合理映射则使用目标文件；目录请求可为空字符串
    - `line: 0`
    - `message: "compile request timed out in queue after 5000ms"`
- `dev_test` 请求若因排队超时失败：
  - 返回现有 `CompileServiceResponse` 顶层结构。
  - `ok` 为 `false`。
  - 保持 `kind: "dev_test"`。
  - 使用现有 `error` 对象通道表达失败原因。
  - 新增稳定的 `error.type = "queue_timeout"`。
  - `error.message` 写明等待超过 5000ms。
- 本地连接失败仍然保持客户端错误路径，不纳入排队超时语义。

**客户端行为：**
- `lpccp` 命令行参数解析不变。
- `lpccp` 仍然执行：
  1. 计算或解析 `id`
  2. 连接 pipe
  3. 发送现有请求 JSON
  4. 阻塞等待服务端返回最终 JSON
- `lpccp` 不需要理解新的顶层字段，因为本设计不新增顶层字段。
- 退出码保持当前语义：
  - `0`：返回 `ok: true`
  - `1`：服务已处理请求，但返回 `ok: false`，包括编译失败、`dev_test` 失败和队列超时
  - `2`：连接失败或本地请求级错误

**服务端生命周期与关闭行为：**
- `driver` 启动 compile service 的时机保持不变。
- `driver` 停止 compile service 时：
  - 停止接受新的连接或新入队请求。
  - 队列中尚未开始执行的请求应收到稳定失败响应，而不是让客户端无限阻塞。
  - 正在执行中的请求允许完成，或按现有退出路径安全终止；无论采用哪种策略，都必须保证客户端最终得到响应或连接明确关闭。

**实现建议：**
- 在 `compile_service.cc` 中引入：
  - `std::mutex`
  - `std::condition_variable`
  - `std::deque<PendingRequest>`
  - 单独的 worker thread
- 接入线程负责：
  - 接受连接
  - 读取完整请求
  - 创建 `PendingRequest`
  - 入队
  - 等待该请求被执行或被判定超时
  - 将最终 JSON 回写给对应客户端
- worker 负责：
  - 从队列中按 FIFO 取任务
  - 判断是否已超过 5000ms
  - 若未超时，调用现有 `execute_compile_service_request()`
  - 若超时，构造兼容旧协议的失败响应
- 第一版不要求一个 pipe 连接承载多次请求；仍保持“一次连接只处理一次请求”。

**测试策略：**
- 增加服务端并发接入测试：两个客户端同时请求同一个 `driver`，两者都能收到最终响应。
- 增加串行执行顺序测试：第二个请求只会在第一个请求完成后才进入实际执行区。
- 增加 compile 排队超时测试：第一个请求阻塞超过 5 秒，第二个 compile 请求返回 `ok: false`，并在 `diagnostics` 中带有超时信息。
- 增加 `dev_test` 排队超时测试：第一个请求阻塞超过 5 秒，第二个 `dev_test` 请求返回 `ok: false` 且 `error.type == "queue_timeout"`。
- 增加“执行时间超过 5 秒但不超时”测试：请求在 5 秒内开始执行后，即使总执行时间超过 5 秒，也应正常完成。
- 增加 FIFO 测试：多个等待请求按进入顺序执行。
- 增加服务关闭测试：停止服务时，未开始执行的等待请求不会无限阻塞。

**权衡：**
- 该方案不能提升单个 `driver` 的真实编译吞吐，但能显著改善并发调用时的稳定性与工具集成体验。
- 通过保持 CLI 和最终 JSON 形状不变，可以最大限度减少已接入工具链的改造成本。
- 固定 5 秒排队超时能防止请求无限堆积，但也意味着长时间占用 worker 的任务会让后续请求失败；这是第一版可接受的简单策略。
- 未来如果编辑器保存风暴成为主要场景，可以在后续版本中增加同目标去重或覆盖策略，但不纳入本设计。

**预期源码变更：**
- 修改 `D:/code/fluffos/src/compile_service.cc`
  - 从“单连接即执行”改为“接入层 + 单 worker 队列执行”。
- 可能修改 `D:/code/fluffos/src/compile_service.h`
  - 若需要暴露更多服务状态或辅助接口。
- 可能修改 `D:/code/fluffos/src/main_lpccp.cc`
  - 若为了更稳地等待最终响应需要微调客户端阻塞或错误处理，但不得改变 CLI 和 JSON 输出形状。
- 修改 `D:/code/fluffos/src/tests/test_compile_service.cc`
  - 增加并发排队、超时和关闭路径测试。

**验收标准：**
- 现有 `lpccp` 命令行调用方式不变。
- 现有成功 JSON 响应形状不变。
- 现有普通失败 JSON 响应形状不变。
- 两个或多个 `lpccp` 可并发请求同一个 `driver`。
- `driver` 内部仍只会串行执行一个 compile-service 请求。
- 排队等待超过 5 秒的请求会稳定失败，不会无限阻塞。
