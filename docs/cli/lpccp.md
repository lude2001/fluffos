---
layout: doc
title: cli / lpccp
---

# cli / lpccp

`lpccp` 是 FluffOS 运行时编译服务与开发测试入口的本地客户端。

它不会自己启动新的 driver，也不会脱离运行中的 mud 环境单独执行。  
它的职责是连接一个已经启动好的 `driver`，请求这个 driver 在当前运行环境里：

- 重新编译并装载指定的 LPC 文件或目录
- 或调用某个对象暴露的固定开发入口 `dev_test()`

然后把结构化结果输出为 JSON。

## 使用前提

1. 必须先启动一个运行中的 `driver`
2. `lpccp` 传入的 config 路径必须和这个 `driver` 启动时使用的是同一个配置文件
3. 当前实现是本地 IPC，用于本机开发环境
4. 同一个 `driver` 允许多个 `lpccp` 并发发起请求，但请求在 `driver` 内部仍按串行顺序执行

## 用法

普通编译：

```bash
./lpccp <config-path> <path>
```

普通编译支持三种运行时状态策略：

```bash
./lpccp --reload-loaded <config-path> <path>
./lpccp --compile-only <config-path> <path>
./lpccp --fresh-required <config-path> <path>
```

其中：

- `--reload-loaded` 是默认行为，会尝试销毁并重新装载已加载对象
- `--compile-only` 不会尝试替换已加载对象；如果目标已经加载，会明确返回 `reload_loaded_object_failed`
- `--fresh-required` 要求目标当前未加载；如果已经加载，会提示需要重启 runtime 后再验证

开发测试：

```bash
./lpccp --dev-test <config-path> <path>
```

调试模式也支持直接传内部 `id`：

```bash
./lpccp --id <id> <path>
```

开发测试模式同样支持直接传内部 `id`：

```bash
./lpccp --dev-test --id <id> <path>
```

其中：

- `<config-path>` 是启动目标 `driver` 时使用的配置文件路径
- `<path>` 可以是：
  - 单个 LPC 文件，例如 `/single/master.c`
  - 一个目录，例如 `/single/`
  - 一个实现了 `dev_test()` 的对象，例如 `/single/tests/dev/dev_test_success.c`

## 推荐工作流

Windows 下建议优先使用 [`build.cmd`](/D:/code/fluffos/build.cmd) 产出 [`build/dist`](/D:/code/fluffos/build/dist)：

```powershell
build.cmd
```

然后：

1. 启动 `driver`
2. 再调用 `lpccp`

例如 testsuite 环境：

```powershell
cd D:\code\fluffos\testsuite
D:\code\fluffos\build\dist\driver.exe etc\config.test
```

普通编译：

```powershell
D:\code\fluffos\build\dist\lpccp.exe D:/code/fluffos/testsuite/etc/config.test /single/master.c
```

目录编译：

```powershell
D:\code\fluffos\build\dist\lpccp.exe D:/code/fluffos/testsuite/etc/config.test /single/
```

运行开发测试：

```powershell
D:\code\fluffos\build\dist\lpccp.exe --dev-test D:/code/fluffos/testsuite/etc/config.test /single/tests/dev/dev_test_success.c
```

## 返回格式

`lpccp` 的标准输出始终是 JSON。

当前协议无论是普通编译还是 `--dev-test` 请求，都会尽量返回同一组顶层字段。  
区别在于某些字段只对特定请求类型有意义。

### 顶层公共字段

| 字段 | 类型 | 含义 |
|------|------|------|
| `version` | `number` | 协议版本，当前为 `1` |
| `kind` | `string` | 请求类型，例如 `file`、`directory`、`dev_test` |
| `target` | `string` | 本次请求的目标路径 |
| `ok` | `boolean` | 本次请求是否整体成功 |
| `phase` | `string` | 失败阶段，例如 `connect`、`target`、`compile`、`reload`、`runtime`、`dev_test` |
| `reason` | `string` | 结构化失败原因；`ok: false` 时一定非空 |
| `message` | `string` | 面向人的失败说明；`ok: false` 时一定非空 |
| `diagnostics` | `array` | 编译相关诊断列表 |
| `runtime_errors` | `array` | 本次请求生命周期内捕获到的运行时错误 |
| `output` | `array<string>` | 运行期间捕获到的输出。普通编译通常为空数组，`--dev-test` 会返回 `write` / `printf` 等输出 |
| `results` | `array` | 目录级逐文件结果。非目录请求通常为空数组 |

普通编译请求还会带：

| 字段 | 类型 | 含义 |
|------|------|------|
| `files_total` | `number` | 目录请求的 `.c` 文件总数；单文件请求通常为 `0` |
| `files_ok` | `number` | 目录请求成功文件数；单文件请求通常为 `0` |
| `files_failed` | `number` | 目录请求失败文件数；单文件请求通常为 `0` |
| `summary` | `object` | 目录请求的失败原因聚合计数 |

常见 `reason` 包括：

- `syntax_error`
- `target_not_found`
- `unsupported_target_kind`
- `reload_loaded_object_failed`
- `runtime_error`
- `service_error`
- `compile_timeout`
- `service_busy`
- `pipe_connect_failed`
- `timeout`

`--dev-test` 失败时会带：

| 字段 | 类型 | 含义 |
|------|------|------|
| `error` | `object` | 结构化错误信息 |

`--dev-test` 成功时会带：

| 字段 | 类型 | 含义 |
|------|------|------|
| `result` | `object` | `dev_test()` 返回 JSON 字符串解析后的对象 |

### 单文件编译

成功示例：

```json
{
  "diagnostics": [],
  "files_failed": 0,
  "files_ok": 1,
  "files_total": 1,
  "kind": "file",
  "message": "",
  "ok": true,
  "output": [],
  "phase": "",
  "reason": "",
  "results": [],
  "runtime_errors": [],
  "target": "/single/master.c",
  "version": 1
}
```

判断逻辑建议：

1. 先看 `ok`
2. 如果 `ok: false`，先看 `reason` / `phase` / `message`
3. 再看 `diagnostics` 和 `runtime_errors`

失败时，`diagnostics` 中会包含结构化诊断，例如：

```json
{
  "severity": "error",
  "file": "/single/bad_example.c",
  "line": 12,
  "message": "syntax error"
}
```

单条 `diagnostics` 的字段说明：

| 字段 | 类型 | 含义 |
|------|------|------|
| `severity` | `string` | 诊断级别，通常是 `warning` 或 `error` |
| `file` | `string` | 诊断所属文件 |
| `line` | `number` | 对应源文件行号 |
| `message` | `string` | 诊断文本 |

### 目录编译

目录请求会返回聚合结果：

```json
{
  "version": 1,
  "ok": false,
  "kind": "directory",
  "target": "/single/",
  "files_total": 3,
  "files_ok": 2,
  "files_failed": 1,
  "message": "1 of 3 file(s) failed",
  "output": [],
  "phase": "compile",
  "reason": "directory_compile_failed",
  "results": [
    {
      "file": "/single/a.c",
      "ok": true,
      "diagnostics": [],
      "message": "",
      "phase": "",
      "reason": "",
      "runtime_errors": []
    },
    {
      "file": "/single/b.c",
      "ok": false,
      "diagnostics": [
        {
          "severity": "error",
          "file": "/single/b.c",
          "line": 8,
          "message": "syntax error"
        }
      ],
      "message": "syntax error",
      "phase": "compile",
      "reason": "syntax_error",
      "runtime_errors": []
    }
  ],
  "runtime_errors": [],
  "summary": {
    "files_failed": 1,
    "files_ok": 2,
    "files_total": 3,
    "reload_failed_count": 0,
    "runtime_error_count": 0,
    "service_error_count": 0,
    "syntax_error_count": 1,
    "unsupported_count": 0
  },
  "diagnostics": []
}
```

判断逻辑建议：

1. 先看顶层 `ok`
2. 再看 `files_failed`
3. 用 `summary` 快速判断失败类别
4. 最后遍历 `results` 中每个失败文件的 `reason` / `message`

### Header 目标

`.h` 文件不是独立编译目标。直接传 header 会返回结构化失败：

```json
{
  "kind": "header",
  "ok": false,
  "phase": "target",
  "reason": "unsupported_target_kind",
  "message": "header files are not standalone compile targets",
  "target": "/include/example.h"
}
```

### 开发测试请求

`--dev-test` 模式不会做任意函数调用。  
它只会在目标对象上调用一个固定的、零参数的入口：

`dev_test()`

这个入口的约定是：

1. 目标对象自行构造场景、执行动作、读取状态
2. `dev_test()` 必须返回一个 JSON 字符串
3. `lpccp`/driver 会把这个 JSON 字符串解析后放进响应里的 `result`
4. 函数运行期间的 `write` / `printf` 等输出会被收集到 `output`

成功示例：

```json
{
  "diagnostics": [],
  "files_failed": 0,
  "files_ok": 0,
  "files_total": 0,
  "kind": "dev_test",
  "ok": true,
  "output": [
    "setup complete",
    "running assertions: 1"
  ],
  "result": {
    "ok": 1,
    "summary": "basic dev test passed",
    "checks": [
      { "name": "sanity", "ok": 1 }
    ],
    "artifacts": {
      "value": 1
    }
  },
  "results": [],
  "runtime_errors": [],
  "target": "/single/tests/dev/dev_test_success.c",
  "test_status": "ok",
  "version": 1
}
```

失败示例：

```json
{
  "diagnostics": [],
  "error": {
    "message": "dev_test exploded\n",
    "type": "runtime_error"
  },
  "files_failed": 0,
  "files_ok": 0,
  "files_total": 0,
  "kind": "dev_test",
  "message": "dev_test exploded\n",
  "ok": false,
  "output": [
    "before failure"
  ],
  "phase": "runtime",
  "reason": "runtime_error",
  "results": [],
  "runtime_errors": [
    {
      "error_type": "runtime_error",
      "message": "dev_test exploded\n",
      "trace": []
    }
  ],
  "target": "/single/tests/dev/dev_test_runtime_error.c",
  "compile_status": "ok",
  "test_status": "failed",
  "test_message": "dev_test exploded\n",
  "version": 1
}
```

当前 `reason` / `error.type` 常见值包括：

- `compile_failed`
- `test_missing`
- `test_failed`
- `runtime_error`

当目标对象没有 `dev_test()` 时，不会被伪装成编译失败：

```json
{
  "ok": false,
  "kind": "dev_test",
  "compile_status": "ok",
  "test_status": "missing",
  "phase": "dev_test",
  "reason": "test_missing",
  "message": "Object does not define dev_test()"
}
```

## 连接失败

如果 `lpccp` 无法连接到对应的编译服务，标准输出仍会返回 JSON，`reason` 会区分：

- `service_busy`
- `pipe_connect_failed`
- `timeout`

常见原因包括：

- 目标 `driver` 还没有启动
- `lpccp` 使用的 config 路径和 `driver` 启动时使用的不是同一个
- `driver` 启动失败，没有真正进入可用状态
- 并发请求过多，客户端在自动 retry/backoff 后仍未连上 pipe

## 退出码

- `0`：请求成功，且返回 `ok: true`
- `1`：请求已送达，但编译失败、`dev_test` 失败，或请求在排队阶段等待超过 5 秒后失败，返回 `ok: false`
- `2`：本地连接或请求级错误，例如无法连接到编译服务

## 适用场景

- 编辑器保存后触发单文件编译
- 本地工具做目录级批量检查
- 直接执行某个 probe / daemon / 开发对象暴露的 `dev_test()`
- 为 IDE、LSP 或自定义开发工具提供运行时编译/开发测试后端

## 注意事项

- `lpccp` 走的是运行中的 FluffOS VM 语义，不是独立离线编译器语义
- 普通模式请求的是“真实重编译并装载”，不是单纯静态语法检查
- 多个 `lpccp` 可以同时发起请求，但同一个 `driver` 内部只会串行执行一个 compile-service 请求
- 若请求在 `driver` 内部排队等待超过 5 秒仍未开始执行，会按普通失败响应返回，而不是无限阻塞
- 一旦请求开始执行，就不会再受这 5 秒排队超时限制
- `--dev-test` 不支持任意函数调用，只会调用固定的 `dev_test()`
- `--dev-test` 的 `output` 用于辅助看过程，最终结论应以 `result` 或 `error` 为准
- 因此它更适合本地开发环境，而不是生产环境
