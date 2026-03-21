# 运行时 LPC 编译服务设计

**目标：** 通过由 `driver` 持有的本地 IPC 服务，把 FluffOS 的真实 LPC 编译与装载行为暴露给本地开发工具，使外部程序可以请求单文件或目录级重编译，并拿到结构化诊断结果，而不需要为每次请求临时启动一套独立环境。

**背景：**
- LPC 的“编译”语义由运行中的 FluffOS 虚拟机和当前 mud 环境共同定义，而不是一个完全独立的离线编译器。
- 一次有效的编译请求依赖当前 config 文件建立起来的真实运行上下文，包括 mudlib 根目录、include 路径、master object、simul_efun object 以及启用的 package。
- 如果每次请求都临时拉起一个新的进程，既慢、又浪费，也不能准确反映开发者真正关心的“当前运行中的 driver 环境”。

**核心决策：**
- 编译服务由运行中的 `driver` 进程直接持有。
- 新增一个本地命令行客户端 `lpccp`，通过本地 IPC 向正在运行的 `driver` 发送编译请求。
- 用户侧命令直接传入 config 路径；客户端内部再把 config 文件身份转换成稳定 `id`。
- 编译行为采用当前虚拟机中的真实重编译与重装载语义，而不是单独再做一条纯分析路径。

**非目标：**
- 第一版不做 TCP 或 HTTP 传输。
- 不通过 mudlib 的 socket 或 HTTP 层转发编译请求。
- 不做每次请求都单独启动一次 `lpcc` 风格的一次性进程。
- 不做依赖图分析器，也不做并行编译调度。

**标识模型：**
- 每个运行中的 driver 实例都由其规范化后的 config 文件路径唯一标识。
- 编译服务基于该 config 路径生成稳定的 `id`。
- 当前仓库约定保证同一份 config 文件不会被两个 driver 同时使用，因此 config 身份足够作为实例身份。
- `lpccp` 对用户暴露的是 config 路径，而不是 `id`；客户端根据 config 路径在本地计算出同样的 `id`，再解析对应的 IPC 端点。
- `id` 是传输层和服务端内部使用的实现细节，同时保留一个显式 `--id` 调试入口即可。

**传输方式：**
- 仅使用本地 IPC。
- Windows 第一版使用 named pipe：
  - `\\\\.\\pipe\\fluffos-lpccp-<id>`
- driver 在配置加载完成后创建该管道。
- 客户端建立连接，发送一次请求，接收一次响应，然后断开。

**请求模型：**
- 主命令形式：
  - `lpccp <config-path> <path>`
- 可选调试形式：
  - `lpccp --id <id> <path>`
- `<path>` 可以是：
  - 单个 LPC 源文件
  - 需要递归编译的目录
- 客户端在本地把 `config-path` 规范化并转换成稳定 `id` 后，再连接对应 pipe。
- 进程内部请求负载统一使用 JSON：

```json
{"version":1,"op":"compile","target":"/adm/foo/bar.c"}
```

或者：

```json
{"version":1,"op":"compile","target":"/adm/foo/"}
```

**响应模型：**
- 服务始终返回 JSON。
- 单文件编译响应：

```json
{
  "version": 1,
  "ok": false,
  "kind": "file",
  "target": "/adm/foo/bar.c",
  "diagnostics": [
    {
      "severity": "warning",
      "file": "/adm/foo/bar.c",
      "line": 12,
      "message": "Unused local variable 'x'"
    }
  ]
}
```

- 目录编译响应：

```json
{
  "version": 1,
  "ok": false,
  "kind": "directory",
  "target": "/adm/foo/",
  "files_total": 12,
  "files_ok": 10,
  "files_failed": 2,
  "results": [
    {
      "file": "/adm/foo/a.c",
      "ok": true,
      "diagnostics": []
    },
    {
      "file": "/adm/foo/b.c",
      "ok": false,
      "diagnostics": [
        {
          "severity": "warning",
          "file": "/adm/foo/b.c",
          "line": 12,
          "message": "Unused local variable 'x'"
        }
      ]
    }
  ]
}
```

**编译语义：**
- 该服务在当前运行中的 driver 内执行真实的运行时重编译。
- 该服务不使用 `reload_object()`，因为 `reload_object()` 只会重置运行时状态，不会重新解析源码，也不会载入新的程序。
- 对每个目标文件：
  - 先把目标路径规范化为 driver 使用的 object path。
  - 如果对象已经加载，则通过 `find_object2()` 找到它。
  - 用 `destruct_object()` 销毁已加载对象。
  - 调用 `remove_destructed_objects()` 刷新待析构队列。
  - 再调用正常的对象加载路径重新编译并装载。
- 编译成功意味着当前运行中的 driver 已经持有新的对象版本。
- 编译失败意味着该对象在请求结束后可能处于未加载状态；对开发环境来说这是可以接受的行为。

**目录编译语义：**
- 递归扫描目标目录下所有 `.c` 文件。
- 按字典序稳定排序后逐个编译，确保执行顺序可预测。
- 每个文件都复用同一套单文件真实重编译逻辑。
- 某个文件失败时不中断整个目录请求。
- 最终把所有文件的结果汇总到同一个 JSON 响应里。

**诊断收集：**
- 复用 driver 当前已有的编译诊断链路，而不是再发明一套独立 warning 系统。
- `smart_log()` 仍然是编译 warning 和 error 的权威出口。
- 在一次 IPC 编译请求期间，driver 挂载一个请求级别的 diagnostics collector。
- `smart_log()` 继续保留当前行为：
  - 输出普通 debug 日志
  - 调用 `master->log_error`
- 同时在 collector 存在时，附加写入一条结构化诊断记录。
- 这样既不会改变现有运行时行为，也能把同样的信息交给外部工具消费。

**并发模型：**
- 编译请求串行处理。
- 当前 FluffOS 的编译链路不是可重入的，所以并行编译显式不在第一版范围内。
- 如果服务在一个请求还未完成时收到新请求，第一版优先采用简单串行队列，而不是复杂的并发执行模型。

**失败处理：**
- IPC 连接失败由 `lpccp` 以客户端错误返回。
- 无效 `id` 表示找不到对应的本地 pipe 端点。
- 非法路径、权限拒绝、文件缺失、语法错误等，根据发生位置返回为正常诊断或请求级致命错误。
- 目录请求即使部分文件失败，也要保留并返回已经完成的部分结果。

**实现职责划分：**
- `driver`
  - 负责服务启动与关闭
  - 负责解析 config-path 对应的 `id`
  - 负责监听 named pipe
  - 负责执行编译请求
  - 负责收集诊断结果
- `lpccp`
  - 负责解析命令行参数
  - 负责根据 config 路径计算 `id`
  - 负责根据 `id` 解析 named pipe 名称
  - 负责发送单次编译请求
  - 负责打印 JSON 响应
  - 负责根据结果设置进程退出码

**预期源码变更：**
- 新增 compile service 模块，承载 IPC 服务行为。
- 新增请求级 diagnostics collector，用于收集结构化编译信息。
- 在 driver 初始化和退出流程中挂接 compile service 的启动与关闭。
- 新增 `lpccp` 可执行程序入口。
- 新增单文件运行时重编译辅助逻辑，负责“强制销毁 + 真实重载”。
- 新增目录遍历与聚合逻辑。

**测试策略：**
- 为 diagnostics collector 的格式化逻辑补单元测试。
- 为单文件 IPC 编译流程补集成测试。
- 为目录编译聚合结果补集成测试。
- 为“源码修改后确实重新编译，而不是仅仅 reset”补回归测试。
- 为语法错误返回结构化诊断补失败路径测试。

**权衡：**
- 采用真实运行时重编译，能够保证语义和 LPC 实际运行方式一致，但也意味着开发态编译请求会影响当前对象状态。
- 这个代价是可接受的，因为该特性明确面向本地开发环境，而不是生产运行时隔离场景。
- 不再额外维护一条纯分析路径，也让实现更小、更贴近开发者平时看到的真实行为。

**进入实现阶段时沿用的前提假设：**
- 编译服务仅面向本地开发环境。
- 一份 config 文件只对应一个运行中的 driver 实例。
- Windows named pipe 是第一版目标传输方式。
- 现有由 LPC pragma 控制的 warning 行为保持不变，服务只负责把 driver 实际发出的诊断结构化输出。
