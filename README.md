# FluffOS 独立开源 Windows 运行时分支

本仓库是一个基于 FluffOS 的独立开源驱动项目，主要关注 Windows 运行时、
生产部署、LPC 开发工作流、与 `lpc-support` 的协同，以及经过筛选的官方
bugfix 回移。

原始 FluffOS 项目仍然是驱动谱系的来源，但本仓库不是官方 FluffOS
发布渠道。我们刻意保持这个分支独立，不是为了封闭开发，而是为了让本仓库
的开源协作、自动化、AI 辅助开发、release、issue 处理等行为都停留在本仓库
边界内，避免意外影响官方维护者或官方基础设施。

## 项目边界

- 可写仓库仅限 `origin`，当前为 `lude2001/fluffos`。
- 对官方 `fluffos/fluffos` 的访问仅限只读调研和选择性回移。
- 不要新增 `upstream` git remote，也不要依赖 upstream tracking branch。
- 本仓库可以接受围绕自身目标的开源协作；但不要向任何非 `origin` 仓库发起
  Pull Request、推送分支、编辑 release，或修改任何远端状态。
- 每次选择性合并官方功能时，都必须更新
  [`docs/superpowers/upstream-merge-tracker.md`](docs/superpowers/upstream-merge-tracker.md)。

## 开源协作

欢迎围绕本仓库目标进行 issue、讨论、补丁和功能改进。适合本仓库的贡献通常
包括：

- Windows 构建、打包、安装和运行体验改进。
- LPC 开发工具链、`lpccp`、`lpcprj` 和编译服务相关改进。
- 与本分支实际使用场景匹配的 driver bugfix。
- 从官方 FluffOS 只读调研后，经过审慎筛选的小步回移。
- 本仓库文档、测试和本地开发体验改进。

如果某个改动更适合官方 FluffOS，请优先尊重官方项目自己的维护流程；本仓库
不会通过自动化替官方发起 PR 或修改官方仓库状态。

## 与 lpc-support 的协同目标

本仓库和 `lpc-support` 是围绕 LPC 开发体验协同工作的两个开源项目：

- 本仓库负责 driver、运行时、编译语义、`lpccp`/`lpcprj`、Windows 产物和
  实际运行环境。
- `lpc-support` 负责 VS Code 侧的 LPC 语言体验，例如语法、诊断、跳转、
  补全、格式化、项目索引和编辑器工作流。
- 两者之间应保持稳定、可解释的协作边界：driver 提供真实编译/运行反馈，
  editor tooling 消费这些能力并服务日常开发。
- 涉及 LPC 语义、诊断格式、编译服务协议、路径规则、配置文件约定和运行时
  行为的改动，应同时考虑 `lpc-support` 的兼容性和演进方向。

这个协同关系是本仓库存在的重要目标之一：我们希望 LPC 开发者既能获得真实
driver 语义，又能在编辑器里得到高质量的开发反馈。

## 开发环境与生产环境

虽然本仓库是开源的，但它不是只用于实验或开发环境。我们自己也会将它用于
实际运行场景，因此这里同时维护两类目标：

- 开发环境：快速编译、热重载辅助、`lpccp`/`lpcprj`、编辑器协同、测试和
  诊断效率。
- 生产环境：稳定的 driver 行为、可复现的 Windows 构建、清晰的发布产物、
  谨慎的官方修复回移，以及对运行时兼容性的保守处理。

因此，本仓库的改动不能只以“开发方便”为唯一标准；凡是会影响生产运行、
协议行为、mudlib 兼容性、构建产物或长期维护成本的改动，都需要按生产使用
的标准审慎评估。

## 本分支关注什么

- 以 Windows 为优先目标的可运行发布目录，统一分阶段放入 `build/dist`。
- 以 `build.cmd` 作为 Windows 构建和打包的规范入口。
- 将 `driver.exe`、`lpccp.exe`、`lpcprj.exe`、运行时 DLL、`include/`、
  `std/` 和 `www/` 放在一起，方便本地实际使用。
- 改善 LPC 开发工作流，尤其是通过 `lpccp`/`lpcprj` 使用持久编译服务。
- 与 `lpc-support` 保持协同，让 driver 的真实语义能够服务编辑器诊断和
  开发体验。
- 同时面向开发环境和生产环境，避免为了短期工具便利破坏运行稳定性。
- 在价值明确、风险可控时，选择性合并官方运行时修复和 efun。

## 当前已选择性回移的官方内容

官方最新快照、已合并内容和仍未合并功能记录在：
[`docs/superpowers/upstream-merge-tracker.md`](docs/superpowers/upstream-merge-tracker.md)。

当前已选择性回移的内容包括：

- `request_clean_up()` 和 `set_clean_up()`。
- 带显式配置白名单的 `get_os_env()` 和 `set_os_env()`。
- `member_array()` 谓词模式，以及反向搜索未命中修复。
- `call_out` handle 查询/移除修复。
- `pcre_match_all()` 零宽匹配处理，避免无限循环。
- prompt 路径错误隔离，以及空错误上下文保护。
- SQLite 执行失败后的清理修复，避免重复 `sqlite3_finalize()`。
- `sprintf` 列模式缩进换行修复。
- `restore_variable()` 宽而浅的 array/mapping 恢复修复，以及尺寸缓存扩容
  溢出保护。
- 长局部变量名分配修复，避免编译器固定 block 写越界。
- `call_stack(4)` 统一返回 `file:line`。
- `int op= float` 和声明为 `float` 的复合赋值语义修复。
- `unique_mapping()` 在 callback 或映射构造异常路径上的释放修复。
- 对象加载深度计数提前到 `valid_read` 之前，避免递归加载绕过深度保护。
- parser package 保留 UTF-8 多字节 token，修复非 ASCII `STR`/`OBJ` 匹配。
- class body 后直接跟变量名时给出更明确的编译诊断。
- `safe_apply()` 和函数指针回调异常路径的 value stack 回滚修复。
- async 文件/数据库回调与 DNS resolve 回调在配置允许时保留
  `this_player()`。
- `to_buffer()`、buffer 字节数组语义、buffer range 赋值、buffer `foreach`
  和 byte ref 行为。
- PR #1247 第一批部分安全修复：`write_buffer()` 边界保护、`read_file()`
  终止符边界保护、反向 `strsrch()` 非边界匹配保护、`sys_reload_tls()`
  端口索引检查、socket 地址/accept 边界修复，以及 MySQL BLOB 按行长度
  读取。
- PR #1247 第二批部分安全/边界修复：随机数非正边界、`pcre_replace()`、
  `uncompress()`、`terminal_colour()`、`repeat_string()`、`replace_string()`、
  `get_dir()`、`add_action()`、`call_out()`、matrix、mudlib stats、macro 参数
  解析，以及 `INT_MIN` 除法/取模常量折叠保护。
- PR #1247 第三批部分 async 安全修复：async worker in-flight 请求引用标记、
  `async_getdir()` 目录项缓冲尺寸修复，以及 async read/write/getdir 拒绝路径
  下的 callback 释放。
- PR #1247 第四批部分交互输入安全修复：`input_to()` 拒绝指向 `#` 开头的
  内部 apply，并在退出前释放已挂起的输入回调和 carryover 参数。
- PR #1247 第五批部分 external 安全修复：`external_start()` 在
  `posix_spawn()` 失败时清理已 provision 的 efun socket，避免留下半开 fd、
  event listener 和 callback 字符串。
- PR #1247 第六批部分 `call_other()` 安全修复：type-check 只按声明参数
  数量扫描 `argument_types`，并避免把对象路径/函数名拼出的错误信息当作
  printf format 使用。
- PR #1247 第七批部分 parser 生命周期修复：active parse 期间延迟释放
  verb node，并在 handler 中途 destruct 时清理半成品结果，避免
  mid-parse use-after-free / null object 调用。
- PR #1247 第八批部分运行时硬化修复：`allocate(n, function)` 异常路径、
  telnet LINEMODE/ZMP 边界、`query_replaced_program()` 无参分支、
  `replaceable(ob, ({}))`、mapping compose key 释放、`sprintf()` 大 precision、
  disassembler switch table、trace/compiler 格式串、`strftime()` 栈空间、
  checkmemory 默认失败消息、`norm()` helper 长度来源，以及 MUD 端口输入长度
  边界。
- PR #1247/#1258 的 compiler hardening 增量：本地旧 compiler 额外的
  `yywarn()` 格式串保护、local-name 缓冲复制路径对齐，以及本地
  `MAXLINE` 边界下的长局部名回归测试。
- PR #1239/#1241 的预处理 directive 注释修复：directive 行内 `/*...*/`
  按空白处理、跨物理行 block comment 不泄漏为代码，并补充 `#define`、
  `#ifdef`、`#undef`、`#if` 的本地回归测试。
- PR #1230 的编译期 master applies：`inherit_program()` 和
  `include_file()`，支持 inherit/include 重定向、内联源码和拒绝路径，并补充
  本地 compiler hook 回归测试。

更大的官方工作不会自动合并，例如编译器前端现代化、官方 VS Code 扩展、
`recompile_object()`、FFI、WebAssembly 目标，以及 CI/release 流程重构。

## Windows 构建

在仓库根目录运行：

```powershell
.\build.cmd
```

本地 Windows 构建唯一支持的输出目录是：

```text
build/dist
```

构建成功后，请使用 `build/dist` 中的产物，不要从临时构建目录运行。

通常会分阶段生成：

- `driver.exe`
- `lpccp.exe`
- `lpcprj.exe`
- 运行时 DLL 依赖
- `run-driver.cmd`
- `include/`
- `std/`
- `www/`

## 运行

直接运行驱动：

```powershell
.\build\dist\driver.exe D:\path\to\config.cfg
```

或使用项目启动器：

```powershell
.\build\dist\lpcprj.exe D:\path\to\config.cfg
```

通过本地编译客户端编译或测试 LPC 文件：

```powershell
.\build\dist\lpccp.exe --help
```

## 测试

Windows 下的主要验证方式：

```powershell
.\build.cmd
cd testsuite
..\build\dist\driver.exe etc\config.test -ftest
```

如果要做更聚焦的 LPC 编译服务检查，可以使用 `lpccp`，传入对应的 driver
配置和目标 LPC 对象。

## 核心能力

本分支保留 FluffOS 驱动基础：

- 基于 MudOS/FluffOS 谱系的 LPC VM 和编译器。
- UTF-8 感知的字符串操作。
- TLS 和 WebSocket 支持。
- 异步 IO 和数据库包。
- 模块化 package/efun 实现。
- 覆盖大量 efun 和运行时行为的 testsuite。
- 继承自 FluffOS 的内存诊断和调试工具。

本分支额外关注的实用能力：

- Windows 发布目录分阶段和安装器导向的打包。
- `lpccp` 编译服务客户端。
- `lpcprj` 本地项目/运行时启动器。
- 与 `lpc-support` 协同的真实编译反馈和编辑器开发工作流。
- 面向生产使用的 Windows runtime 产物和保守运行时修复。
- 位于 `docs/superpowers/` 下的本地计划和规格文档。

## 依赖

仓库内置或 vendored 的依赖包括：

- libwebsockets
- libevent
- backward-cpp
- musl `crypt`
- ghc::filesystem
- nlohmann::json
- scope_guard
- utfcpp
- utf8_decoder
- 带本地修改的 libtelnet

平台依赖包括 ICU、OpenSSL、zlib、数据库客户端库，以及 Windows 上的
MSYS2/MINGW64 工具链。

## 文档

- 构建和运行时说明：[`docs/`](docs/)
- CLI 说明：[`docs/cli/`](docs/cli/)
- 本地计划和规格：[`docs/superpowers/`](docs/superpowers/)
- 官方合并追踪：
  [`docs/superpowers/upstream-merge-tracker.md`](docs/superpowers/upstream-merge-tracker.md)

## 与官方 FluffOS 的关系

本项目存在，是因为 FluffOS 本身很有价值。我们保持独立分支，是为了避免
本仓库的运行决策、开源协作方式和 AI 辅助工作流的失误污染官方维护。

对官方 FluffOS 应当保持尊重：只读调研，谨慎回移有价值的内容，并清楚记录
哪些已经合并、哪些仍未合并。
