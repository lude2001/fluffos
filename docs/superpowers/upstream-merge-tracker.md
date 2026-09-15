# FluffOS 官方版本合并跟踪表

本文档跟踪对官方 `fluffos/fluffos` 仓库的只读审查，并记录哪些上游功能已经合入本独立分支。

今后每次合并或回移官方功能，都必须在同一变更集中更新本文档。此流程不得添加名为 `upstream` 的 Git 远程；对官方仓库的访问始终保持只读。

## 当前上游快照

- 官方仓库：`fluffos/fluffos`
- 已审查的官方版本：`v2026.0901.0`（附注标签）
- 稳定版提交：`7af5c3fffe2505c7cb764951bed863b16eee471b`
- 稳定版原始提交标题：`tracing: stop a trace that fills its buffer from wedging tracing for good (#1361)`
- 稳定版提交时间：`2026-08-31T21:17:12-07:00`
- 审查日期：`2026-09-15`
- 迁移分支：`codex/rebase-v2026.0901.0`
- 旧版独有功能抽离归档标签：`legacy/extension-extraction-2026-09-15`，指向 `a3cbf2bf5815d4fa269712fb4e9c37a2773d4b56`
- 最近一次本地合并提交：`00cf1f218f14efbcfb55dfb63bd9b5bb4c046497`（`merge upstream int division edge guards`）
- 上一次本地合并提交：`58a4be68929a97ead0b37834866766f6b60b5e71`（`merge upstream varargs parameter guard`）

## 以 `v2026.0901.0` 为基线重建

新的本地候选分支直接从官方稳定版标签解引用后的提交开始，而不是重放本分支原有的 130 个本地提交。独有能力以小型、可审查的层次重新引入。首个候选版本保留：

- 原生 JSON 包；
- HTTP 辅助函数、请求和响应解析包；
- Windows `build/dist`、启动器、安装器和分阶段打包层；
- 本仓库的四个轻量 CI/CD 工作流，包括静态生产 driver 工件；
- 运行时编译服务和独立 `lpccp`，通过较小的编译器/VM 边界接入。

首个候选版本有意不保留本地 mapping 插桩，因为江湖 mudlib 中没有非测试消费者。官方编译器、VM、解析器、async、FFI、WASM、网络和第三方代码继续作为权威实现；不能仅因旧回移实现与官方不同就重新引入。

Windows/MSYS2 MinGW64 上的纯官方基线验证：

- 使用与官方 Windows CI 相同的包配置完成 RelWithDebInfo 配置、构建和安装。
- 340 个非 testsuite CTest 用例全部通过。
- 因为有一个现存本地 driver 被有意保留运行，LPC testsuite 无法使用硬编码的 `4000` 至 `4003` 端口。复制 testsuite 并把配置及两处端口断言改为 `24000` 至 `24003` 后，711 个文件中的 10,655 项检查全部通过，并输出 `Checks succeeded.`。
- 禁用 MySQL、选择 SQLite 时，配置过程会输出 `FATALPACKAGE_DB_DEFAULT_DB is not valid!`，但仍以成功状态退出；MinGW 也会报告官方及 vendored 源码中已有的警告。这些属于官方基线现象，不是本地迁移回归。

截至 `89c107ec` 的本地候选版本验证：

- 规范 Windows 入口 `build.cmd` 成功完成，并生成 `build/dist`、安装映像和双语安装器。目录布局、相对配置启动器、安装器配置以及用户 PATH 安装/卸载检查均通过。
- 366 个非 testsuite CTest 用例全部通过。最终代码生成的 `build/dist/driver.exe` 又在隔离的 testsuite 副本中通过了 715 个文件、10,690 项 LPC 检查。
- JSON 包通过 32 项原生检查，官方 LPC JSON 套件保留其 178 项检查；HTTP 辅助函数、请求和响应测试通过 90 项检查。
- 修复命名管道启动就绪竞态后，26 项编译服务测试全部通过。该扩展现为 CMake 可选项（默认关闭），仅由本地 Windows 构建/CI 启用；管道 DACL 仅允许当前用户、SYSTEM 和 Administrators。针对隔离江湖运行时的真实 `lpccp` 请求返回结构化成功结果，diagnostics 和 runtime errors 均为空，并覆盖相对 driver 配置路径与绝对客户端配置路径组合。
- 安装映像通过 `lpcprj` 冷启动江湖 mudlib 副本，复制的 `gameteststd1` 存档能够登录并执行 `look`。新 driver 暴露了 mudlib 的两个假设，已在 LPC 仓库分别修复：为 `START_ROOM` 显式包含 `login.h`，以及在自由任务前预加载背包分类表/服务。两者均未转化为 driver 兼容补丁。
- 同一套 32 项原生 JSON 契约在旧 driver 和候选 driver 上均通过。复制的江湖存档完成“旧版读取→候选版恢复/保存→旧版再次读取”，两个方向都能登录并执行 `look`。
- 同一候选运行时中的两次登录覆盖了断线和重连路径。90 秒有界观察期间未出现新的编译、运行时或存档错误，常驻内存从 80,896 KiB 变为 80,308 KiB。这是本地迁移证据，不能替代未来的生产环境长期观察。
- 最终干净源码工件从 `a3860bcc` 重新构建，版本为 `20260820-dd2a3a14-a3860bcc`。SHA-256：`driver.exe` 为 `c438b3d5d964bc1c561073753ff159dfa3e62fd11bffa47ea334a4ad21a9104d`，`lpccp.exe` 为 `37a2c52ba81b0ccb3d593c05651777e064832c26341dee0ea801a4503d5a0af6`，`lpcprj.exe` 为 `8475446e4deadbf0225ef45c94786fe53ebc44537d6e6c86e2f8dd684334ab4e`，安装器为 `5f8dc5fc8917ccce6efaf0b73aace1795747483f42ab785253de0ee807e76cde`。

任何发布决策前仍需完成：Linux/静态 CI、sanitizer、TLS/WebSocket 和证书验证数据库连接检查、更广泛的玩法对比，以及生产时长级别的稳定性观察。这些工作需要另行授权的发布任务，不是完成当前本地基线重建分支的前置条件。

本次未添加任何远程，也没有推送、发布、部署或访问线上服务器。

## 合入提交 `c20b15e4`

选择性合入或手工回移了以下官方变更：

- 官方问题修复中的 `request_clean_up()` efun。
- 官方清理调度功能中的 `set_clean_up()` efun。
- 带显式读写配置白名单的 `get_os_env()` 和 `set_os_env()` efun。
- `member_array()` 的 flag 4 谓词模式，以及反向搜索未找到结果的修复。
- 针对较旧待处理句柄的 `call_out` 查找/移除修复，包括 union owner 防护。
- 处理 `pcre_match_all()` 零宽匹配，避免无限循环。
- prompt 路径错误隔离和空错误上下文防护。
- SQLite 执行失败后的清理，避免重复调用 `sqlite3_finalize()`。
- 保留缩进的 `sprintf` 列模式换行修复。
- 为上述 efun 和问题修复补充文档与测试。

验证命令：

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check`

备注：`testsuite/single/tests/efuns/db.c` 中已有 SQLite 回归测试，但本次验证使用的 Windows 构建未启用 `__USE_SQLITE3__`，因此跳过了 SQLite 部分。

## 合入提交 `c168e3aa07dcfbd193485a51a7387ce055bf7412`

选择性合入或手工回移了以下官方修复：

- PR #1256：`restore_variable()` 现在限制真实递归嵌套深度，而非累计容器数量，因此宽而浅的数组/mapping 可以正确恢复。
- PR #1258（部分）：编译器本地名称分配器可处理超过常规 4096 字节块的标识符，不再越界。
- PR #1258（部分）：`restore_variable()` 的大小缓存扩容会检测有符号整数溢出，并干净地终止预扫描。
- PR #1244（部分）：`call_stack(4)` 对每个栈帧一致返回 `file:line`。
- PR #1244（部分）：算术复合赋值对声明为 `float` 的左值保持浮点语义，并把运行时 `int op= float` 的结果提升为 float。
- PR #1238（部分）：`unique_mapping()` 在回调或 mapping 构造路径经 `error()` 展开时，会释放已复制的键和部分 mapping。

验证命令：`.\build.cmd`、`..\build\dist\driver.exe etc\config.test -ftest`，以及针对相关编译器、VM、测试文件的 `git diff --check`。

备注：Windows 构建根据 `grammar.y` 重新生成 `grammar.autogen.cc`，生成的行表变化符合预期。PR #1258 的 `recompile_object()` 当时不适用于本地源码树，因为该分支尚无此 efun 入口。

## 合入提交 `09ec81cbf4775ca1972c514f35f075611cec25a7`

选择性合入或手工回移了以下官方问题修复：

- PR #1238 剩余运行时范围：对象加载计数/深度统计提前到 `valid_read` 之前，防止递归 `valid_read` 加载路径绕过保护。
- PR #1238 剩余运行时范围：parser 包分词时保留 UTF-8 多字节，用于 word、`STR` 和 `OBJ` 匹配。
- PR #1244（部分）：class 主体后直接跟变量名时给出有针对性的诊断，而不是含混的通用解析错误。
- PR #1244（部分）：`safe_apply()` 和函数指针回调异常路径从保存的调用基址展开值栈，不再重复弹出参数。
- PR #1244（部分）：启用 `this_player in call_out` 兼容选项时，异步文件/数据库回调和 DNS resolve 回调会保留 `this_player()`。

验证命令：`.\build.cmd`、完整 LPC testsuite 和相关文件的 `git diff --check`。

备注：Windows 构建根据 `grammar.y` 重新生成了 `grammar.autogen.cc/.h`，解析表变化符合预期。新的 async `this_player()` 回归测试有意保持无副作用；本地 testsuite 会编译并调度这些路径，更广泛的回调执行覆盖仍来自既有 async 测试。

## 合入提交 `9ddb8d623bfe972f64d7681400ce498b01199932`

选择性合入或手工回移了官方 PR #1250 的运行时/编译器行为：

- 为字符串、buffer 和整数数组增加公开 `to_buffer()` 与内部 `_to_buffer()` 转换支持。
- 允许 buffer 与 buffer、字符串、整数数组拼接及复合赋值，同时保持严格字节转换。
- 允许以 buffer、字符串或整数数组进行 buffer 范围赋值。
- 对 buffer 字节写入和字节左值算术严格执行 `0..255` 范围，不再静默截断。
- 增加 buffer `foreach` 支持，包括 `foreach(int ref c in buffer_value)` 的可写字节引用。
- 活跃左值/引用直接携带字节指针，减少对全局字节左值状态的使用。
- 本分支保守维持字符串 `foreach ref` 行为：现有测试仍断言字符串引用不会修改源字符串。
- 补充 buffer 字节范围、范围赋值、foreach 和引用的专项运算符测试。

验证：`build.cmd`、四个专项测试、完整 LPC testsuite 和 `git diff --check` 均通过。

备注：未引入 PR #1250 的文档/侧边栏变更，因为本分支使用本地中文文档和不同的文档边界。官方拆分后的编译器前端文件在本分支不存在，相关行为已适配旧版 `grammar.y` / `lex.cc` 布局；生成的解析表变化符合预期。

## 合入提交 `726e990d17b11614ac9387c4b60cdc7f77bf9d73`

作为第一批局部安全修复，选择性回移了官方 PR #1247：

- `write_buffer()` 拒绝负长度，并在不发生有符号溢出的情况下检查 offset/length 边界。
- `read_file()` 在正向行扫描和最大长度限制后，把终止符写入位置限制为实际读取字节数。
- 反向 EGC 搜索在偏移 0 发现未对齐匹配时不再死循环。
- `sys_reload_tls()` 按 `external_port` 条目数量验证端口，而不是把索引与数组字节大小比较。
- `lpcaddr_to_sockaddr()` 拒绝会溢出固定 host 缓冲区的主机名。
- `socket_accept()` 对已接受的 OS socket fd 应用 close-on-exec，并在失败路径关闭 fd。
- MySQL 二进制/字符串字段按当前行实际 BLOB 长度分配缓冲区，不再使用整列最大长度。
- 增加 `write_buffer()` 边界、反向 `strsrch()` 和无效 `sys_reload_tls()` 端口索引回归测试。

验证：`build.cmd`、三个专项测试、完整 LPC testsuite（日志 `build/lpc-full-test-pr1247-partial.log`）和 `git diff --check`。

备注：这是 PR #1247 的部分合入。该批次刻意限定为不需要引入官方专属编译器布局、热重载或大规模包重构的低风险运行时安全修复。

## 合入提交 `9414cfe9bb775ef10b2c3bfe3188c09aeebf7f86`

作为第二批安全/边界修复，选择性回移了官方 PR #1247：

- `random_number()` 和 `secure_random_number()` 在构造随机分布前，对非正边界返回 `0`。
- 共享字符串哈希表大小增长会在有符号整数溢出前停止 2 的幂增长循环。
- `pcre_replace()` 的计数和复制阶段使用相同的非重叠捕获组选择，避免嵌套捕获导致堆覆盖。
- `uncompress()` 在 inflate 失败时释放部分输出。
- `terminal_colour()` 使用 `safe_apply()` 调用 `terminal_colour_replace()`，使回调错误不泄漏本地缓冲区。
- `repeat_string()` 在字符串长度乘以重复次数前执行限制。
- `replace_string()` 对 Boyer-Moore skip-copy 快速路径进行边界检查和计数。
- `get_dir()` 在 `stat()` 前检查目录/文件路径拼接边界。
- `add_action()` 的缺失函数错误使用有界格式化，且不再把用户文本作为格式串。
- `call_out()` 在回收时处理空函数指针 owner，并对“秒转毫秒”执行饱和转换。
- 矩阵变换拒绝少于 16 个元素的数组。
- mudlib stats 的 author/domain 及状态文件恢复路径限制固定缓冲区复制/扫描。
- 宏参数解析遇到非法参数名即停止，不再带着未消费字符继续。
- 常量折叠的 `INT_MIN / -1` 和 `% -1` 在 `#if`、除法及取模折叠中避免 C++ 未定义行为。
- 增加 PCRE、随机数、矩阵、超长 call_out、非法宏参数、terminal_colour 回调、uncompress 清理及 64 位整数除法/取模回归测试。

验证：`build.cmd`、相关专项测试、完整 LPC testsuite（日志 `build/lpc-full-test-pr1247-second-batch.log`）和 `git diff --check`。

备注：仍为 PR #1247 的部分合入；`grammar.autogen.cc` 由 `grammar.y` 重新生成，变化符合预期。

## 合入提交 `ed01cbc055924f13df67cd4bd62795db2a96defb`

作为第三批，选择性回移了官方 PR #1247 的 async 修复：

- 已从队列弹出但尚未移入完成队列的 async worker 请求会记录在 `current_works`，使 debugmalloc 标记能够覆盖回调函数指针和捕获的 `command_giver`。
- `async_getdir()` 每个条目按 `sizeof(struct dirent)` 扩容原始目录条目缓冲区，而不是 `sizeof(dirent *)`。
- `async_read()`、`async_getdir()`、`async_write()` 在权限拒绝且不会入队的路径释放回调函数对象。

验证：`build.cmd`、`async_this_player.c`、完整 LPC testsuite（日志 `build/lpc-full-test-pr1247-async.log`）及 `git diff --check`。

备注：仍为 PR #1247 的部分合入。单独运行 `/single/tests/efuns/async.c` 输出 `Checks succeeded`，但在本工具环境超时前没有退出，因此未作为本提交的干净门禁信号。

## 合入提交 `e4eda115aa6d42adc384920f49be49b435a51d9c`

作为第四批，选择性回移了官方 PR #1247 的交互输入修复：

- `input_to()` 在准备 VM 调用帧前拒绝函数名以内部 apply 标记 `#` 开头的字符串回调；拒绝路径会释放待处理 input sentence、引用对象和附带参数，不再从后续栈设置路径直接返回。

验证：`build.cmd`、`input_to.c`、完整 LPC testsuite（日志 `build/lpc-full-test-pr1247-input-to.log`）及 `git diff --check -- src/comm.cc`。

备注：仍为 PR #1247 的部分合入。专项测试覆盖既有非交互设置路径；`#` apply 分支属于交互安全路径，因此以 Windows 构建、既有 `input_to` 测试和完整 LPC testsuite 为门禁，而非专门的真实 socket 回归。

## 合入提交 `b6a529611506f9350877d859d1039f6edb424732`

作为第五批，选择性回移了官方 PR #1247 的外部进程 socket 清理：

- `posix_spawn()` 在 socket 注册后失败时，`external_start()` 通过 `socket_close(fd, SC_FORCE | SC_FINAL_CLOSE)` 关闭已提供的 efun socket，然后清空 `sv[0]`，避免延迟原始 fd 清理重复关闭。
- 通过 `socket_efuns.h` 暴露 `socket_close()` 内部 flag，使 external 包可使用与 socket 包相同的强制最终关闭路径。

验证：`build.cmd`、`sockets.c`、完整 LPC testsuite（日志 `build/lpc-full-test-pr1247-external.log`）和相关文件的 `git diff --check`。

备注：仍为 PR #1247 的部分合入。受影响的 `posix_spawn()` 失败路径位于非 Windows 实现；本地 Windows 验证证明共享 socket API 暴露及既有行为仍能构建并通过，但不是 POSIX spawn 失败清理路径的专项运行时复现。

## 合入提交 `94c3029bee39d9528c9243debc1757c85c8f429f`

作为第六批，选择性回移了官方 PR #1247 的 `call_other()` 类型检查修复：

- `check_co_args()` 读取 `prog->argument_types` 时向 `check_co_args2()` 传递有界的声明参数数目，同时保留完整入栈参数数目用于栈索引；额外实参不再导致类型检查器越过声明参数类型表读取。
- `call_other()` 类型检查错误以 `error("%s", buf)` 输出，不再把对象派生文本当作 printf 格式串。
- 既有 `call_other` efun 回归测试会临时开启运行时类型检查并覆盖额外参数错误路径。

验证：`build.cmd`、`call_other.c`、完整 LPC testsuite（日志 `build/lpc-full-test-pr1247-call-other.log`）及相关文件的 `git diff --check`。

备注：仍为 PR #1247 的部分合入。

## 合入提交 `797ca93624c7543a54ba3e453d0f96af25be5f2a`

作为第七批，选择性回移了官方 PR #1247 的 parser 生命周期修复：

- 活跃解析中被移除的 parser verb node 延迟到最外层解析退出后释放，避免 handler 在 `can_`/`direct_`/`do_` apply 中销毁自身或调用 `parse_remove()` 后，`parse_vn` 指向已释放内存。
- `clear_result()` 将每个已保存参数数目初始化为 0，确保错误清理不读取未初始化计数。
- `we_are_finished()` 提交候选项时若 handler 已销毁，则释放半成品结果并清空 `best_match`，避免以空/已销毁目标调用 `do_the_call()`。
- 增加 `/single/tests/crasher/parser_handler_destruct.c`，覆盖 `direct_*` 返回成功时 handler 自毁。

验证：`build.cmd`、两个专项测试、完整 LPC testsuite（日志 `build/lpc-full-test-pr1247-parser-uaf.log`）和相关文件的 `git diff --check`。

备注：仍为 PR #1247 的部分合入。首次并行专项测试超时后遗留了本地 `build/dist/driver.exe`，已按精确路径停止后成功重跑 `parse_utf8.c`；未触碰独立的生产路径 `D:\code_env\FluffOS\libexec\fluffos\driver.exe` 进程。

## 合入提交 `a8fada2406154b2ab0942b85d00388114d29d730`

作为第八批运行时加固，选择性回移了官方 PR #1247：

- `allocate(n, function)` 在逐元素回调期间把部分构建的结果数组保留在 VM 栈上，回调错误不再泄漏结果或已保存的引用计数值。
- Telnet LINEMODE 子协商在读取/回显第二个子选项字节前检查其存在；ZMP 参数数组填充 `item[0..n-1]`，不再越界一格。
- 无显式对象的 `query_replaced_program()` 读取 `current_object->replaced_program`，不再把任意栈顶值当对象。
- `replaceable(ob, ({}))` 即使调用者忽略列表为空也会分配内置忽略项。
- `compose_mapping()` 释放节点前会释放被删除的 mapping key。
- `sprintf()` 对用户提供的超大精度使用有界 `snprintf()` 输出整数/浮点数。
- 反汇编器把直接 switch 表的 `minval` 视为 `sizeof(LPC_INT)`，不再硬编码 4 字节。
- 编译器重载警告和 trace 行以 `"%s"` 参数传递源代码派生文本，而非作为格式串。
- `strftime()` 改用堆存储，不再使用由 `__MAX_STRING_LENGTH__` 决定大小的栈 VLA。
- `checkmemory` 限制默认失败消息的复制长度。
- `norm()` 测量长度时使用传入 helper 的数组，修复 `angle()` 共用路径。
- MUD 端口输入把读取长度限制为本地缓冲区大小，并拒绝非正长度前缀。
- 增加 `replaceable_empty.c`，扩充 `allocate` 和 `sprintf` 回归测试。

验证：`build.cmd`、相关专项测试、完整 LPC testsuite 及 `git diff --check`。

备注：仍为 PR #1247 的部分合入；完整 testsuite 状态为 0，输出仅在终端观察，未重定向到跟踪工件。

## 合入提交 `1b03c79fa98733e8b7265f56eb4d27bb5003b017`

将官方 PR #1239/#1241 的预处理器指令注释行为选择性适配到本分支旧版 `src/compiler/internal/lex.cc`：

- 预处理器指令负载中的块注释折叠为空白而非完全删除，使 `1 -/* comment */-1` 之类宏体保持 token 边界。
- 跨物理行的指令行块注释仍作为指令的一部分消费，避免续行文本被当作普通 LPC 代码分词。
- 通过 `#define`、`#ifdef`、`#undef` 和嵌套宏参数展开测试固定本地既有的指令负载尾部行注释行为。
- 增加 `/single/tests/compiler/preprocessor.c`，覆盖块注释尾部、有效/无效 `#if` 分支、包含注释标记的字符串字面量、注释作为空白的 token 分隔、尾部 `//` 宏注释及带尾注释的 `#ifdef`/`#undef` 查找。

验证：`build.cmd`、`preprocessor.c` 和相关文件的 `git diff --check`。

备注：这是 PR #1239/#1241 行为对本地布局的回移，不是整体引入官方 `lexer_rules_pp.cc`、生成的 Flex scanner 或官方 GTest 编译器框架。首次专项测试因 dist 中仍为旧 driver 而失败；重新构建后通过。

## 合入提交 `56c00880b40692bbc12a803026dd739e042859f2`

将官方 PR #1230 的编译期 master apply 行为选择性适配到本分支旧编译器/lexer 布局：

- 增加 `inherit_program(string from, string path, int priv)` master apply，每条 LPC `inherit` 语句都会查询它。
- `inherit_program()` 可保留默认路径、重定向到其他程序、以字符串数组提供内联源码或拒绝继承。
- 增加 `include_file(string compiled, string from, string path)` master apply，每条 `#include` 都会查询它。
- `include_file()` 可保留默认路径、重定向到其他 include、以字符串数组提供内联内容或拒绝 include。
- 增加 `StringLexStream`，使 master 提供的内联 include/inherit 文本通过与磁盘文件相同的 lexer stream 抽象编译。
- 增加 `load_object_from_source()` 用于合成继承程序，并支持正常的继承父级重试流程，包括内联源码继续继承未加载的磁盘或合成父级。
- testsuite master 通过 `set_compile_hooks()` 转发 hook，供专项编译器测试使用。
- 增加继承重定向、内联继承程序、拒绝继承、private inherit flag、include 重定向、内联 include、嵌套 include 调用形态和嵌套内联继承源码的测试/fixture。

验证：`build.cmd`、四组专项编译器测试、完整 LPC testsuite 和 `git diff --check`。

备注：核心行为已适配本地 `grammar_rules.cc` / `lex.cc` 布局，没有整体导入官方文件。PR #1230 的官方热重载 daemon/demo 不在此提交中，后来以 `.c` testsuite 布局在 `362f6fefae58197c27baa7243ac494db6882c9dd` 引入。当 mudlib master 返回原路径或不路由到自定义 hook 时，新 apply 保持原生产行为。

## 合入提交 `99aa8be9e5db99f26d503bec7beae6ff32856921`

将官方 PR #1247/#1258 的编译器加固项选择性协调到本分支旧 `lex.cc` / `compiler.cc` 布局：

- 本地旧编译器的 `define_new_function()` 有一个额外的预组装警告缓冲区，可包含源代码派生的函数/程序名；现改为 `yywarn("%s", buff)`，不再把缓冲区当 printf 格式串。
- 本地名称分配器的普通块路径使用 `memcpy()` 复制已计算的字节长度，不再用 `strcpy()` 重新扫描。
- 增加 `/single/tests/compiler/long_local_name.c`，覆盖接近 `MAXLINE` 的本地/全局名称编译路径。

验证：`build.cmd`、相关专项测试和文件级 `git diff --check`。

备注：这不是 PR #1247 或 #1258 的完整合入。当时 #1258 的 `recompile_object()` 悬空文件名指针修复仍待 PR #1237 引入该 efun。由于旧 lexer 先限制 `MAXLINE=4096`，官方 `>4096` 标识符回归无法直接用本分支 LPC 源码表达；新增测试固定了不改变行长语义时可编译的最大源码级情形。

## 合入提交 `fa228dce2b4ca9eb7f7219474dac2b4014d2d13d`

把官方 PR #1244 的字符串语义覆盖选择性合入既有 `string_index.c` 回归套件：

- 记录在 `strlen(s)` 位置索引字符串会返回虚拟 NUL 的预期行为。
- 增加 CRLF 扩展字素簇覆盖：`strlen("\r\n") == 1`；索引 `"\r\n"` 时簇 0 返回 CR codepoint、簇 1 返回虚拟 NUL；`strsrch()` 只在簇边界匹配。

验证：`string_index.c` 及相关文件的 `git diff --check`。

备注：这是 PR #1244 行为的测试/注释合入；运行时行为已存在于本分支，测试用于防止未来把 CRLF 误解为两个可搜索/索引的簇。

## 合入提交 `06719fab6466eed700d3bc6474b23dd02c27adca`

选择性合入官方 PR #1237 的热重载基础：

- 对象全局变量改存于独立 `TAG_OBJ_VARS` 分配块，不再位于 `object_t` 尾部；该块至少包含一个 `svalue_t`，通过 `allocate_object_variables()` 分配。
- `object_t` 增加 `prog_generation`，供后续 `recompile_object()` 在程序交换后使旧函数指针失效。
- debug memory 检查从所属对象标记对象变量块，并显式报告孤立 `TAG_OBJ_VARS` 块。

验证：`build.cmd`、完整 LPC testsuite 和相关文件的 `git diff --check`。

备注：这是 PR #1237 的部分合入，尚未公开 `recompile_object()` LPC API。生产 mudlib 仍通过 `ob->variables` 使用同样变量；变化只在 driver 内部。剩余工作包括 efun、实时程序交换、按名称迁移变量、master/simul_efun、函数指针代际检查、执行帧防护、clone/virtual/call_out/heart_beat 安全处理和专项测试。

## 合入提交 `3ebd18267178a324798779a4ae19be90306fe0dc`

选择性合入官方 PR #1237 的函数指针基础：

- 函数指针 header 在创建或 `bind()` 时记录 owner 的 `prog_generation`。
- owner 已切换到较新程序代际时，`call_function_pointer()` 拒绝旧 `FP_LOCAL` 和 `FP_FUNCTIONAL` 指针。
- `FP_LOCAL` 保存创建时的 `program_t` 并对该程序持有/释放 `func_ref`，不再在交换后递减 owner 当前程序。
- debug memory 检查把 `FP_LOCAL` 的额外函数引用计入保存的创建程序。
- `%O` 使用保存的创建程序打印本地函数指针，并容忍已移除的 simul_efun 表项。

验证：`build.cmd`、完整 LPC testsuite 和相关文件的 `git diff --check`。

备注：仍为 PR #1237 的部分合入，尚未公开 `recompile_object()`，但已保证后续程序交换中的函数指针生命周期和旧布局检查安全。现有生产 mudlib 无需改动，只有后续热重载实际提升程序代际时行为才会变化。

## 合入提交 `721fa0e65938b5b60fcb71fe6b8b82078ee5d552`

选择性合入官方 PR #1237 的 `recompile_object()` 核心行为：

- 增加公开 `recompile_object(object)` efun。
- 重编译 master copy 时，把新程序原地交换到在线 master copy 及其已加载 clone。
- 全局变量按名称迁移，兼容源码变更后保留既有状态；新增变量保持初始化器值。
- 执行重编译程序的 `__INIT` 前重建 master apply 和 simul_efun 分发表。
- 程序替换时启用此前基于 `prog_generation` 的旧函数指针保护。
- 在交换点取消待处理的 `replace_program()` 状态。
- 拒绝不安全目标，包括 clone、旧程序当前位于 VM 执行栈的对象、嵌套 `recompile_object()`、以及重编译前已有待处理 `replace_program()` 的对象。
- 增加 master/clone 更新、按名称迁移变量、新变量初始化器、旧本地函数指针及拒绝 clone 目标的专项回归。

验证：`build.cmd`、`recompile_object.c`、完整 LPC testsuite 和 `git diff --check`。

备注：这是 PR #1237 核心 efun/交换机制对本地布局的合入，并非整体引入官方全部 demo、文档或大型 fixture。现有生产 mudlib 只有主动调用 `recompile_object()` 或依赖新热重载行为时才需调整。

## 合入提交 `9e2aed146a581a951c7e79bb153c7d583761bca1`

PR #1237 适用后，选择性合入官方 PR #1258 的 `recompile_object()` Coverity 修复：

- 编译器使用稳定的 `old_prog->filename` 指针，不再传入临时本地字符串缓冲区。
- 通过 `DEFER` 关闭重编译打开的源码 fd，避免编译错误路径泄漏。

验证：`build.cmd`、`recompile_object.c`、完整 LPC testsuite 和 `git diff --check -- src/vm/internal/simulate.cc`。

## 合入提交 `1b8c5cc68c32380d80b4f19f81456c09f62239ce`

将官方 PR #1237 回归覆盖选择性适配到本分支 `.c` testsuite：

- 扩充 `recompile_object.c`，覆盖源码消失、虚拟对象后备源码重编译、交换期间的 `replace_program()` 状态、自毁或报错的 `__INIT`、在线 simul_efun 表重建及 master 执行帧拒绝。
- 增加 `recompile_object2.c`，覆盖 catch_tell 路由、shadow 链、按名称 call_out 使用新程序、旧函数指针 call_out 干净失败、add_action sentence 和 heart_beat 注册。
- testsuite master 增加 `compile_object()` fixture，覆盖 `/data/recompile_object/virt` 虚拟对象。

验证：两个专项测试、完整 LPC testsuite 和 `git diff --cached --check`。

备注：本分支单测框架在专项测试后立即关闭，因此未包含官方“运行结束后 idle master 重编译”fixture；本地仍覆盖 master 执行帧防护，交换 master 时也会重建 apply。`recompile_object2.c` 验证同步部分，并验证名称/函数指针 call_out 句柄在重编译后仍存活且可移除，不依赖官方延迟 verifier。

## 合入提交 `362f6fefae58197c27baa7243ac494db6882c9dd`

把官方 PR #1230/#1237 的热重载 demo 和覆盖选择性适配为仅 testsuite 使用的开发示例：

- 增加 `/single/hot_reload.c` daemon：注册为 testsuite master 编译 hook、记录 include/inherit 依赖边、快照源码大小和 mtime，并重载过期的被监视程序。
- 保留状态的重载路径使用 `recompile_object()`，原地更新 master copy 和在线 clone，同时按名称保留兼容变量。
- 不保留状态的路径使用 destruct+load，使 master copy 从初始化器重启；既有 clone 保持旧程序，直到 mudlib 自行迁移或销毁。
- 带 `hot_reload_state()` / `hot_reload_restore()` 的对象使用协作式 destruct+load，并显式决定保留哪些状态。
- 增加 `/single/tests/applies/hot_reload.c`，覆盖依赖图、最深层 include 变更、共享依赖过期集合、编译失败自愈、祖先由深到浅刷新、当前执行防护记录保留、状态保留重载、clone 更新、协作恢复和 opt-out 语义。
- 调整 `recompile_object2.c`，使 call_out 存活覆盖在本地测试运行器中同步执行且不遗留生成文件。

验证：`recompile_object2.c`、`hot_reload.c`、完整 LPC testsuite 和相关文件的 `git diff --check`。

备注：这是 testsuite 开发示例，不是生产默认行为，仅在 testsuite 显式加载和启用后生效。未整体引入官方文档站热重载页面；本地边界记录在此文件和 README。

## 合入提交 `af6ccca5849049377135f204b70d775c301dcf72`

选择性适配了官方 PR #1247 剩余的低风险安全/正确性修复：

- `ed` 在复制到调用者缓冲区前限制展开后的打印行，限制默认文件名复用和输入文件名长度，并为转义替换/模式文本预留足够空间。
- `sprintf()` 列/表格式化持有待处理状态使用的 `%O`/字符串渲染缓冲区，不再借用临时清理缓冲区；展开时释放 pad 和自有存储。
- `restore_variable()` / mapping 恢复路径把非法或过深顶层 mapping 的计数失败报告为恢复错误，并在可能 longjmp 越过正常清理的 `error()` 前清空临时 scratch 状态。
- `replace_dollars()` 按实际写入的替换字符串长度检查输出增长，而非匹配模式长度。
- 增加 `sprintf_column_object.c`，扩充 `restore_variable.c` 的深层构造输入覆盖。

验证：`build.cmd`、相关专项测试、完整 LPC testsuite 和文件级 `git diff --check`。

备注：已在源码适配 `src/packages/dwlib/dwlib.cc`，但当前 Windows 配置未启用 `PACKAGE_DWLIB`；因此该可选包路径仅有代码审查/构建布局覆盖，没有本次真实 LPC 包运行测试。

## 合入提交 `b7e294247ee027115fb9333701b2d3ff29a245b0`

选择性适配了官方 PR #1247 的 reclaim 安全修复：

- `check_svalue()` 超过 `MAX_RECURSION` 提前返回时，`reclaim_objects()` 保持内部 `nested` 递归计数平衡。
- 从 `FP_LOCAL` 函数指针回收已销毁 owner 时不再立即递减程序 `func_ref`；函数指针继续存活，之后由 `dealloc_funp()` 对保存的创建程序恰好释放一次。
- 增加 `/clone/reclaim_fp_helper.c` 和 `/single/tests/crasher/reclaim_funptr_owner.c`，覆盖 owner 已销毁、函数指针仍被保存并安排于 call_out，随后再次由 `reclaim_objects()` 访问的情况。

验证：`build.cmd`、专项测试、完整 LPC testsuite 和相关文件的 `git diff --check`。

## 合入提交 `ae8a687fa34a442c0891c6109ba342e734e84277`

适配了官方 PR #1247 的 buffer 范围回归覆盖：

- 扩充 `/single/tests/operators/buffer_range_assign.c`，覆盖 buffer 右值在增大和缩小方向上的变长范围赋值；由此固定此前已合入的运行时修复：重新分配时从 `buffer_t::item` 而不是 `buffer_t` header 复制。

验证：专项测试、完整 LPC testsuite 和相关文件的 `git diff --check`。

## 合入提交 `bab352bad1679d7f838e72cd9c121faf8008a483`

适配了官方 PR #1247 的 MySQL 回归覆盖：

- 扩充 `/single/tests/efuns/db.c`，即使当前 Windows 构建未启用 SQLite，也能运行 MySQL 部分。
- 增加可选的逐行二进制字段长度覆盖：OS 环境同时存在 `FT_MYSQL_HOST`、`FT_MYSQL_DB`、`FT_MYSQL_USER` 时，测试创建临时 `VARBINARY` 表，并验证 `db_fetch()` 为每行返回按该行实际二进制长度分配的 buffer。
- 把上述简短环境变量名加入 `/testsuite/etc/config.test` 的 `get_os_env()` 白名单；保持短名称以避免超过既有配置行限制。
- 该覆盖有意设为可选。没有 MySQL 凭据的普通开发机或 CI 会跳过真实 MySQL 检查，不使 DB efun 测试失败。

验证：`build.cmd`、`db.c`、`get_os_env.c` 和相关文件的 `git diff --check`。

备注：一次完整 testsuite 运行到达 `db.c` 后因未配置环境变量而跳过可选 MySQL 检查，随后在既有 `async.c` 回调路径因 `async_read()` 返回 `-1` 失败；后者不是本次 MySQL 覆盖变更导致，也未作为本提交门禁信号。

## 合入提交 `c98193933a4abfafd2b65ef2386973cef4630228`

适配了官方 PR #1247 剩余的 parser/socket 安全覆盖：

- `living_parse()` 跳过 `parse_command("%l")` 对象列表中的非对象项，不再将其作为对象解引用；这与官方对调用者传入 `0` 或被 `check_for_destr()` 转为 `0` 的已销毁对象防护一致。
- 增加 `/single/tests/crasher/living_parse_nonobject.c`，覆盖 `%l` 列表中的非对象项。
- 增加 `/single/tests/crasher/socket_long_host.c`，覆盖主机部分超过固定缓冲区时的干净拒绝。底层修复本地早已存在，本提交补充适配本分支 include 布局的测试覆盖。

验证：`build.cmd`、两个专项测试和相关文件的 `git diff --check`。

## 合入提交 `58a4be68929a97ead0b37834866766f6b60b5e71`

把官方 PR #1247 剩余的编译器安全修复适配到本分支旧 `grammar.y` 布局：

- varargs 参数声明读取 `type_of_locals_ptr[max_num_locals - 1]` 前会确认存在前置本地参数。
- `void probe(void ...)` 等非法声明现在报告编译错误，不再读取 `type_of_locals_ptr[-1]`。
- 增加 `/clone/bad_varargs_void.c` 和 `/single/tests/compiler/bad_varargs_void.c` 回归覆盖。

验证：`build.cmd`、相关专项测试和文件级 `git diff --check`。

备注：`grammar.autogen.cc` 由 `grammar.y` 重新生成，生成的行表变化符合预期。

## 合入提交 `00cf1f218f14efbcfb55dfb63bd9b5bb4c046497`

把官方 PR #1247 剩余的整数边界防护适配到本分支旧编译器/VM 布局：

- 常量折叠的 `constant / constant` 和 `constant % constant` 在除数为 `-1` 时避免 `INT_MIN / -1` 的 C/C++ 未定义行为。
- VM 运行时整数 `/`、`%` 使用相同防护：`INT_MIN / -1` 保持二进制补码环绕后的 LPC 结果，`INT_MIN % -1` 返回 `0`。
- `/=` 和 `%=` 复合赋值使用同样防护，避免相同边界条件使左值更新崩溃或触发未定义行为。
- 增加 `/single/tests/operators/int_min_div_mod.c`，覆盖 64 位最小整数的运行时 `/`、`%`、`/=`、`%=`。

验证：`int_min_div_mod.c`、`compound_assign_float.c`、`64bit.c` 和相关文件的 `git diff --check`。

备注：`grammar.autogen.cc` 由 `grammar.y` 重新生成。至此，PR #1247 中适用于本地的低风险 `INT_MIN / -1` 与 `% -1` 防护已覆盖常量折叠、VM 执行和复合赋值。

## `2026-07-14` 完成度审计

提交 `00cf1f218f14efbcfb55dfb63bd9b5bb4c046497` 后，再次以只读方式查询 `fluffos/fluffos` 中的目标 PR，并与本分支当前旧布局源码树比较：

- PR #1247：请求范围内、本地适用的低风险安全/正确性修复均已以本地形式合入，包括分配器/错误路径清理、`sprintf()` 所有权和边界、mapping compose 清理、MySQL 行长度、Telnet LINEMODE/ZMP 防护、trace/编译器/反汇编器加固、`replaceable()` 空忽略列表、`query_replaced_program()` 目标对象、MUD 端口输入边界、parser/socket/reclaim、varargs 和整数边界防护。
- PR #1258：三个适用的 Coverity 修复均已以本地形式合入：lexer 本地名称越界保护、恢复大小溢出保护，以及 PR #1237 使路径适用后的 `recompile_object()` 稳定文件名/fd 清理。
- PR #1239/#1241：指令负载注释剥离和跨物理行的指令块注释已合入旧 lexer 布局，并有本地预处理器回归覆盖。
- PR #1244：请求范围内的源码行为修复均已以本地形式合入，包括 `set_clean_up()`、`call_stack(4)`、class 组合声明诊断、float 复合赋值、async/DNS `this_player()` 保留、安全回调展开和 CRLF/字符串索引覆盖。
- PR #1230/#1237：编译期 master apply、testsuite 热重载 demo、`recompile_object()`、在线 master/clone 程序交换、按名称迁移变量、master/simul_efun 重建、旧函数指针检查、不安全目标防护、虚拟对象、call_out/add_action/heart_beat/shadow 及专项热重载测试均已以本地形式合入。

本次审计对预处理器、inherit/include hook、热重载、`recompile_object()`、恢复、`sprintf`、`replaceable`、async `this_player()`、`call_stack`、class 声明和整数边界等 13 个专项测试进行了验证。

## 已审查快照中尚未合入的内容

截至 `00cf1f21`，以下官方变更仍有意不合入：

- PR #1261：WebAssembly 默认页面的崩溃/错误调试弹窗。
- #1259 之后近期官方第三方依赖更新/清理，包括 fmt、nlohmann/json、utfcpp、libwebsockets 及相关示例/测试树清理。
- PR #1259：官方 `lpc-syntax` VS Code formatter 接线、tokenizer/highlighter 修复、生成的 grammar contract 更新和扩展测试。
- PR #1258 剩余范围：请求的源码行为修复已无已知遗漏；仅官方专属文件布局或测试框架形态仍在本分支之外。lexer 本地名称、恢复溢出和 `recompile_object()` 悬空文件名指针均已以本地形式合入。
- PR #1257 及 #1253-#1255：官方 CI/release 工作流重构和自动发布触发器。
- PR #1250 剩余范围：官方文档/侧边栏更新，以及绑定新版拆分编译器/测试布局的官方专属字符串/引用测试。
- PR #1247 剩余范围：整数边界批次后，请求的低风险源码修复已无已知遗漏；剩余材料仅限官方专属测试、当前 Windows 构建未启用的可选包真实覆盖、FFI 或新版编译器布局专属部分。
- PR #1245：字符模式输入投递改进和登录时 NAWS 修复。
- PR #1244：请求的源码行为修复已无已知遗漏；剩余仅官方文档、依赖文件布局的官方专属用例或测试框架差异。
- PR #1237 剩余范围：官方热重载文档站页面、运行结束后的 idle-master 重编译 fixture 和官方专属测试框架形态。核心 efun、在线 master/clone 交换、变量迁移、master/simul_efun 重建、执行帧防护、clone 目标拒绝、虚拟对象、call_out/add_action/heart_beat/shadow、`replace_program()` 防护、自毁/报错 `__INIT`、旧函数指针保护和 testsuite 热重载 demo 均已以本地形式合入。
- PR #1231 和 PR #1243：WebAssembly driver 目标及 WASM 体积缩减。
- PR #1230 剩余范围：官方文档站页面，以及本分支本地编译 hook/热重载测试未体现的官方专属测试/文档形态。
- PR #1210：大规模 LPC 平台现代化批次，包括基于 Flex 的前端、clang 风格诊断、arena 编译、`.lpc` 源码优先、FFI 包、decimal 库、`tools/lpc-syntax`、官方 VS Code 扩展和生成的语法资源。
- 仅文档更新，例如 Docusaurus/i18n/侧边栏/搜索、大量 efun 文档扩充和文档审计清理。

## 更新规则

今后合入官方功能时：

1. 以只读方式重新查询 `fluffos/fluffos`，并更新上方快照 SHA 和日期。
2. 添加本地提交哈希，以及简明的已合入上游变更列表。
3. 将新合入项目移出“尚未合入”章节；若仅部分合入，则标明剩余范围。
4. 记录验证命令和任何跳过的覆盖范围。
5. 官方仓库始终保持只读：不得为 `fluffos/fluffos` 添加远程、推送分支、编辑 PR 或执行 release 操作。
