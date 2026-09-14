# FluffOS 官方稳定版重基线：本地独有能力清单

**状态：** Task 2 已完成源码与江湖 mudlib 静态审计；Task 3 已完成 JSON 与 HTTP 的首轮模块化抽离
**本地基线：** `98cc9b42b6f7f1630189b4b968ed708ff41f2203`
**官方基线：** `v2026.0901.0`，commit `7af5c3fffe2505c7cb764951bed863b16eee471b`
**共同祖先：** `ee8137603946d12248e9ed429f09eb2388e007e3`

## 结论

当前分叉相对官方稳定版有 130 个本地侧提交、311 个官方侧提交。不能逐提交重放本地历史；真正需要迁移的是下列 6 组本地能力：

1. 原生 JSON efun package。
2. HTTP helper 与请求/响应增量 parser efun package。
3. Windows `build/dist`、`lpcprj`、安装器和打包脚本。
4. 本仓库四个轻量 GitHub Actions workflow，以及静态生产 driver 构建。
5. 运行时编译服务、协议与 `lpccp` 客户端。
6. `mudlib_stats` 中的 mapping 存活数量与创建程序归因；此项没有江湖消费者，降为可选，不进入第一批候选。

其余大量差异属于旧 compiler/VM 布局上的官方回移补丁、旧第三方快照或仓库历史噪声，不作为“独有功能”搬运。

## 独有能力与迁移决定

| 能力 | 官方稳定版 | 江湖实际依赖 | 决定 | 新归属 |
| --- | --- | --- | --- | --- |
| 原生 `json_decode/json_encode/json_format` | 没有 native package；testsuite 有 LPC `std/json.lpc`，且没有 `json_format` | P0：非测试源码分别有 30、100、5 处调用 | 保留完整兼容语义 | 本地独有的 drop-in `src/packages/json/` |
| HTTP helper/parser efun | 没有对应 efun 或实现 | P0：本地 HTTP server/client 直接调用 | 保留完整 package | `src/packages/lude_http/` |
| Windows dist、`lpcprj`、安装器 | 官方没有 | 本地开发与 Windows 交付依赖 | 保留，继续与 VM 解耦 | 根构建入口、`scripts/`、`packaging/windows/`、独立 launcher target |
| 轻量 CI/CD 与静态生产 driver | 官方 workflow 体系不同 | 当前 GitHub 生产 artifact 来源 | 原样保留流程，按新基线做最小参数适配 | 现有四个 workflow |
| runtime compile service 与 `lpccp` | 官方有离线/单进程 `lpcc`，没有连接运行中 VM 的 named-pipe 服务 | 本地开发工具链依赖 | 保留，但抽成可开关 extension；协议中的既有子能力只是兼容范围，不成为本次迁移测试要求 | `src/extensions/lude_compile_service/` + `lpccp` target |
| `MARIADB_TLS_DISABLE_PEER_VERIFICATION=1` | 官方无 `lpcprj` | 本地访问远端数据库的现有兼容条件，尚无移除后的 A/B 实连证据 | 本轮保持，不扩散进 driver 或生产启动路径；证书治理另立任务 | `lpcprj` launcher 内 |
| `mapping_origin_stats()` 与 live mapping 计数 | 无对应 efun | 江湖非测试源码 0 调用 | 第一批不迁移；需要可观测性时再作为独立可选 extension 引入 | 暂不进入候选 |

## 1. 原生 JSON package

### 当前公共接口

```text
mixed json_decode(string)
string json_encode(mixed)
string json_format(string, int | void)
```

当前实现文件：

- `src/packages/json/CMakeLists.txt`
- `src/packages/json/json.cc`
- `src/packages/json/json.spec`
- JSON 专项 testsuite 与文档

官方 tag 中不存在这些 driver package 文件。官方 testsuite 使用 LPC `std/json.lpc` 提供同名函数，但这不证明其错误语义、数字边界、mapping key、循环引用、输出顺序和性能与当前 native efun 等价；`json_format()` 在官方树中完全不存在。

江湖源码审计结果：

| 接口 | 全部调用/定义匹配 | 排除 `test/` 后匹配 | 非测试文件数 |
| --- | ---: | ---: | ---: |
| `json_encode()` | 253 | 100 | 58 |
| `json_decode()` | 38 | 30 | 23 |
| `json_format()` | 5 | 5 | 5 |

迁移约束：公共 efun 名称和现行序列化语义不变；package 自己声明构建 option。官方 `src/packages/CMakeLists.txt` 仍自动发现带 `CMakeLists.txt` 的 package，因此 JSON 可以成为 drop-in package，不需要修改 VM。

## 2. HTTP helper 与增量 parser package

### 当前公共接口

```text
string url_decode(string)
string url_encode(string)
mapping http_decode_query(string)
mapping http_decode_form(string)
string http_build_response(int, mapping, string)
mixed http_parser_create()
mapping http_parser_feed(mixed, string)
void http_parser_close(mixed)
mixed http_response_parser_create()
mapping http_response_parser_feed(mixed, string)
void http_response_parser_close(mixed)
```

当前实现被混在 `src/packages/sockets/`：

- `http_efuns.cc/.h`
- `http_parser.cc/.h`
- `sockets.spec` 中的 11 个接口
- `CMakeLists.txt` 中的 source 列表

官方 tag 中不存在这些文件和 efun。江湖的真实依赖集中在：

- `external_system_package/http/controller/httpd.c`
- `external_system_package/http/controller/http_send.c`

其中 server 直接使用 request parser、`url_decode()` 和 `http_build_response()`；client 直接使用 response parser。`url_encode()`、`http_decode_query()`、`http_decode_form()` 当前没有江湖非测试调用，但属于同一公开 package 且已有测试，首轮一并保留，避免收窄既有 API。

旧基线抽离结果：HTTP 的 11 个 efun、helper 和增量 parser 已移入 `src/packages/lude_http/`；sockets package 不再编译或声明 HTTP 能力。新 package 自己声明 `PACKAGE_LUDE_HTTP`，并补齐 `options.autogen.h` 对 package 清单的增量生成依赖，使 LPC 可见 `__PACKAGE_LUDE_HTTP__`。

迁移约束：重放到官方基线时以独立 `lude_http` package 接入，不修改官方 `sockets.cc`；parser handle 生命周期、分片结果 mapping 和错误结果结构保持兼容。旧基线的 Windows 规范构建和 helper、request parser、response parser 三组定向测试均已通过。

## 3. Windows 构建、launcher 与安装器

官方 tag 中不存在以下本地交付入口：

- `build.cmd`
- `src/main_lpcprj.cc`
- `scripts/stage-driver-dist.ps1`
- `scripts/stage-windows-install-image.ps1`
- `scripts/build-windows-installer.ps1`
- `packaging/windows/fluffos.iss`
- 对应安装布局和相对配置测试脚本

这些代码已经基本位于 VM 外部，应整体保留。新官方工具或 DLL 只通过 staging 发现/复制进入 `build/dist`，不在 driver core 中维护安装器逻辑。

`src/main_lpcprj.cc` 当前设置 `MARIADB_TLS_DISABLE_PEER_VERIFICATION=1` 后再启动 driver。该行为与本地远端数据库可达性相关，本轮不删除、不改名、不移动到全局环境；未来启用 CA/hostname verification 时单独做有/无该变量的本地 A/B 实连。

## 4. 本仓库轻量 CI/CD

必须覆盖官方基线的本地 workflow 只有：

- `.github/workflows/ci-ubuntu.yml`
- `.github/workflows/ci-macos.yml`
- `.github/workflows/ci-windows.yml`
- `.github/workflows/release-artifacts.yml`

不带入官方 CodeQL、Coverity、Docker publish、文档站和大矩阵 workflow。`Release Artifacts` 的静态 Linux job 继续保持 `Release`、`STATIC=ON`、`MARCH_NATIVE=OFF`、MySQL、SQLite、C++ tests、LPC testsuite、启动 smoke、静态链接检查和 `.sha256`。

## 5. runtime compile service 与 `lpccp`

### 当前独有文件

- `src/compile_service.cc/.h`
- `src/compile_service_client.h`
- `src/compile_service_protocol.h`
- `src/runtime_compile_request.cc/.h`
- `src/runtime_dev_test_request.cc/.h`
- `src/main_lpccp.cc`
- `src/tests/test_compile_service.cc`

官方新版 `lpcc` 已拥有 `--batch`、`--json`、tokens、AST 和 bytecode 输出，应直接保留并用于离线分析；它仍然不能替代本地服务连接运行中 VM 后进行 compile/reload 的能力。

当前 compile service 侵入官方 core 的位置可以压缩为：

1. CMake：构建 extension library、`lpccp` executable 和安装目标。
2. `mainlib.cc`：VM 启动后 start，所有退出路径 stop。
3. `backend.cc`：每个 tick 派发有限数量的主线程请求。
4. 一个稳定 adapter：把官方 compiler/VM 内部诊断转换成 extension 自己的协议结构。

Task 3 先做行为保持的目录和 adapter 抽离，不同时改变启用方式。迁到官方基线后再增加显式 build/runtime 开关，使生产 driver 不默认开放 named pipe。本地现有协议、排队、CLI 输出和失败结构先保持兼容。

## 6. mapping 可观测性

本地新增了：

- `mapping_origin_stats()` efun；
- `mudlib_stats_t::mapping_count`；
- 按创建对象程序归因的 mapping 计数；
- `mapping.cc` 分配、复制、释放路径中的计数调用。

江湖非测试源码没有 `mapping_origin_stats()` 调用。它不是 mudlib 启动、协议、存档或数据库依赖，却会触碰官方高风险的 mapping/object 内存结构。因此第一批迁移明确不带入。旧基线代码和专项测试仍由 legacy tag 保存；只有出现明确消费者后，才以默认关闭的 instrumentation hook 单独迁移。

## 官方实现优先，不搬本地回移代码

官方 `v2026.0901.0` 已包含并测试下列当前分叉曾手工回移的能力：

- `recompile_object()`
- `sys_reload_tls()`
- `get_os_env()` / `set_os_env()`
- `request_clean_up()` / `set_clean_up()`
- `to_buffer()` 与 buffer 运算语义
- `memory_summary()`
- 多批 compiler、VM、parser、async、socket、reclaim 和边界修复

这些能力在新分支上一律使用官方源码。旧回移 patch 只作为回归用例来源：官方测试能证明行为时不移植本地实现；只有江湖兼容用例在官方树上复现差异时，才增加最小 adapter 或修复。

官方稳定版还新增了本地旧树没有的 promise/async、FFI、WASM/jsbridge 和新版 compiler frontend。它们是官方基线的一部分，不属于本地扩展，也不得为了降低迁移难度从新基线删除；只通过构建 option 控制不适用的平台功能。

## 明确丢弃的差异

- 旧 compiler layout 对应的手工回移实现和生成文件差异。
- 当前分叉携带的旧第三方源码快照差异。
- 官方重 workflow 与本地轻量 CI 无关的历史。
- 已合入官方稳定版的本地补丁实现。
- 临时 checkpoint、过期设计分支和纯迁移过程文件。
- 第一批候选中的 mapping instrumentation。

## Task 3 抽离顺序

1. `json`：把构建 option 收进 package 自身，使整个 `src/packages/json/` 成为可重放的 drop-in 模块；接口与兼容标记保持不变。
2. `lude_http`：从 sockets 中移出 source/spec，保持接口不变。
3. Windows launcher/build/staging/installer：整理为 VM 外部交付层，并锁定现有测试。
4. CI workflows：只建立迁移覆盖清单，不在旧基线上改流程。
5. compile service：先协议与平台 transport，再 runtime adapter，最后压缩 core hook。
6. mapping instrumentation：不进入本轮抽离提交。

每组能力形成独立提交，禁止把官方回移 patch、格式化或第三方更新混入这些提交。Task 3 的产物应能按上述顺序重放到官方基线。

## Task 2 完成证据

- 官方 tag 通过 URL 只读 fetch 到 `FETCH_HEAD`，未新增 `upstream` remote。
- 官方 tag 对象解析到 commit `7af5c3fffe2505c7cb764951bed863b16eee471b`。
- 已检查独有文件在官方 tree 中不存在。
- 已比较全部 package `.spec` 接口差异。
- 已对江湖 mudlib 统计 JSON、HTTP 与 mapping 统计调用。
- 已把每组差异归类为“保留、官方替代或不迁移”。
