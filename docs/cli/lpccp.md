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

## 用法

普通编译：

```bash
./lpccp <config-path> <path>
```

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
| `diagnostics` | `array` | 编译相关诊断列表 |
| `output` | `array<string>` | 运行期间捕获到的输出。普通编译通常为空数组，`--dev-test` 会返回 `write` / `printf` 等输出 |
| `results` | `array` | 目录级逐文件结果。非目录请求通常为空数组 |

普通编译请求还会带：

| 字段 | 类型 | 含义 |
|------|------|------|
| `files_total` | `number` | 目录请求的 `.c` 文件总数；单文件请求通常为 `0` |
| `files_ok` | `number` | 目录请求成功文件数；单文件请求通常为 `0` |
| `files_failed` | `number` | 目录请求失败文件数；单文件请求通常为 `0` |

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
  "files_ok": 0,
  "files_total": 0,
  "kind": "file",
  "ok": true,
  "output": [],
  "results": [],
  "target": "/single/master.c",
  "version": 1
}
```

判断逻辑建议：

1. 先看 `ok`
2. 再看 `diagnostics`

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
  "output": [],
  "results": [
    {
      "file": "/single/a.c",
      "ok": true,
      "diagnostics": []
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
      ]
    }
  ],
  "diagnostics": []
}
```

判断逻辑建议：

1. 先看顶层 `ok`
2. 再看 `files_failed`
3. 最后遍历 `results`

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
  "target": "/single/tests/dev/dev_test_success.c",
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
  "ok": false,
  "output": [
    "before failure"
  ],
  "results": [],
  "target": "/single/tests/dev/dev_test_runtime_error.c",
  "version": 1
}
```

当前 `error.type` 常见值包括：

- `object_unavailable`
- `missing_entrypoint`
- `runtime_error`
- `bad_return_type`
- `invalid_json`

## 标准错误输出

如果 `lpccp` 无法连接到对应的编译服务，它会把错误写到标准错误，例如：

```text
Error: cannot connect to compile service pipe \\.\pipe\fluffos-lpccp-xxxxxxxxxxxxxxxx (win32=2)
```

常见原因：

- 目标 `driver` 还没有启动
- `lpccp` 使用的 config 路径和 `driver` 启动时使用的不是同一个
- `driver` 启动失败，没有真正进入可用状态

## 退出码

- `0`：请求成功，且返回 `ok: true`
- `1`：请求已送达，但编译失败或 `dev_test` 失败，返回 `ok: false`
- `2`：本地连接或请求级错误，例如无法连接到编译服务

## 适用场景

- 编辑器保存后触发单文件编译
- 本地工具做目录级批量检查
- 直接执行某个 probe / daemon / 开发对象暴露的 `dev_test()`
- 为 IDE、LSP 或自定义开发工具提供运行时编译/开发测试后端

## 注意事项

- `lpccp` 走的是运行中的 FluffOS VM 语义，不是独立离线编译器语义
- 普通模式请求的是“真实重编译并装载”，不是单纯静态语法检查
- `--dev-test` 不支持任意函数调用，只会调用固定的 `dev_test()`
- `--dev-test` 的 `output` 用于辅助看过程，最终结论应以 `result` 或 `error` 为准
- 因此它更适合本地开发环境，而不是生产环境
