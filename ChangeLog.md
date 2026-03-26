================================================================================
  当前 fork 相对官方 master 的差异
================================================================================

本节用于标注当前 fork 与官方 `fluffos/fluffos` `master` 的差异。

- 对比基线：
  - 官方 `master`：`ee8137603946d12248e9ed429f09eb2388e007e3`
  - 当前 fork `origin/master`：`98907f321b916884b9a3a8943e3f991d0d2538cb`
- 对比结果：
  - 当前 fork 相对官方 `master` 额外领先 25 个提交
  - 当前 fork 没有落后于官方 `master`
- 说明：
  - 以下内容基于 `git log upstream/master...origin/master`
  - 这里只统计已经进入 fork `master` 的内容
  - 不包含当前本地开发分支上尚未合并回 fork `master` 的实验性改动

主要差异功能如下：

1. 原生 JSON 能力增强

- 新增原生 `json_encode()` / `json_decode()` efun，由驱动内置实现，不再依赖 mudlib `std/json` 作为唯一实现。
- 新增 `json_format()`，补充 JSON 输出格式化能力。
- 为 JSON 增加了 testsuite 覆盖和速度测试补充。
- 关键提交：
  - `5be9bbd9 feat: add native json efuns`
  - `fe425771 feat: add json_format and clean build warnings`
  - `98907f32 fix: include all mapping buckets in json_encode`

2. HTTP helper / parser 能力扩展

- 在 sockets 包中新增一组 HTTP 工具 efun，包括 URL 编解码、query/form 解码、HTTP 响应构建等辅助能力。
- 新增 HTTP 请求解析器与 HTTP 响应解析器能力，并补齐对应测试与文档。
- 示例层还引入了基于 LPC 的 HTTP 发送侧样例，方便 mudlib 直接消费新能力。
- 关键提交：
  - `d2ef93c0 feat: implement HTTP helper and parser efuns`
  - `bf7a58b7 feat: implement HTTP response parser and related efuns`

3. 运行时 LPC 编译服务与 `lpccp`

- 为驱动增加了运行时 LPC 编译服务的协议和基础服务端骨架。
- 新增 `lpccp` 前端工具与对应 CLI 文档，用于对接这套运行时编译服务。
- 增加了 compile service 的测试与文档设计稿。
- 关键提交：
  - `3135178a feat: add lpccp protocol scaffolding`
  - `8fb59c83 feat: add runtime LPC compile service scaffolding`
  - `96494966 docs: add lpccp usage guide`

4. Windows 本地 dist 工作流增强

- 为 Windows 增加了以 `build.cmd` 为入口的本地构建/分发工作流。
- 新增 `build/dist` staging 逻辑，把 `driver.exe`、`lpccp.exe`、依赖 DLL、`include/`、`std/`、`www/` 收敛成可运行目录。
- 增加了 dist 相对路径启动测试与 VS Code `compile_commands.json` 导出脚本。
- 关键提交：
  - `563efa9f build: stage lpccp with driver dist`
  - `5c8c7551 docs: document preferred dist build workflow`

5. 编译器与兼容性修复

- 修复 inherited prototype 相关的误报告警。
- 调整编译器/语法相关实现，为新增功能和告警清理提供支撑。
- 若干构建告警与第三方依赖兼容性问题被顺带修复。
- 关键提交：
  - `32e6d6bb fix: avoid false inherited prototype warnings`
  - `fe425771 feat: add json_format and clean build warnings`

6. 文档与 fork 工作流约束

- fork 中新增了一组较完整的设计/计划文档，覆盖 HTTP helper、运行时编译服务、原生 JSON、Windows dist 工作流等主题。
- 增加了仅针对 fork 工作流的协作与 GitHub 操作约束文档，例如默认仅允许向 `lude2001/fluffos` 操作、不向上游仓库发 PR/推送等。
- 这些提交主要影响协作流程和文档，不直接改变驱动运行时行为。
- 关键提交：
  - `992fd823 docs: forbid upstream PR and push operations`
  - `2c98af06 docs: default to local-only branch merges`

Wodan 宣布 3.x 版本将由 "Yucong Sun" (sunyucong@gmail.com) 维护。

请提交问题到 https://github.com/fluffos/fluffos

与 2.x 相比的主要更改：

构建：
  * FluffOS 已切换到 C++ 语言，需要 C++11（G++ 4.6+、CLANG 2.9+）来构建。
  * FluffOS 现在使用 C++ try/catch 而不是 setjmp/longjmp 来进行 LPC 错误恢复。（wodan）
  * FluffOS 现在使用 autoconf 进行依赖配置。（alpha4 已完成）
  * 需要 Libevent 2.0+ 来构建，默认使用 epoll() 而不是 select()。（alpha6）
  * 所有 *.c 文件已重命名为 *.cc，所有 *_spec.c 文件已重命名为 *.spec。（alpha6）

EFUN/包更改：
  * 总 EFUN 数量限制已提高到 65535（alpha2）
  * unique_mapping() 不再泄漏内存。
  * 使用 C++11 mt19937_64 作为驱动随机数生成器引擎。random() 的质量得到改善。
  * PACKAGE_CONTRIB：store_variable | fetch_variable 接受对象作为第 3 个最后参数。（Lonely@NT）
  * PACKAGE_CRYPTO：构建修复和增强。（voltara@lpmuds.net）
  * PACKAGE_SHA1：修复不正确的 sha1() 哈希生成，已通过测试验证。（voltara@lpmuds.net）
  * enable_commands() 现在接受一个 int 参数，默认值为 0，与旧行为相同。当传递 1 时，驱动将通过调用其环境、同级、库存对象的 init 来设置动作。（按该顺序）。

新的编译选项/包：
  * PACKAGE_TRIM：（zoilder），字符串修剪的 rtrim、ltrim 和 trim。
  * POSIX_TIMERS：更好的时间精度跟踪用于 eval 成本。（voltara@lpmuds.net）
  * CALLOUT_LOOP_PROTECTION：保护 call_out(0) 循环。（voltara@lpmuds.net）
  * SANE_SORTING：对 "sort_array()" 使用更快的排序实现，但需要 LPC 代码返回一致的结果。
  * REVERSE_DEFER：defer() efun 的 fifo 执行顺序（默认为 lifo）

杂项：
  * FluffOS 现在提供 64 位 LPC 运行时，无论主机系统如何。（包括 32 位 linux/CYGWIN）。
  * addr_server 现在已过时并删除，该功能已内置。（alpha6）
  * DEBUG_MACRO 现在始终为真，该功能正在作为内置功能规范化。
  * 各种错误/崩溃修复。
  * 新的 LPC 预定义 __CXXFLAGS__。
  * /doc 已恢复，内容将针对每个 EFUN/Apply 更改进行更新。

已知问题：
  "-MAX_INT" 在 LPC 中不会被正确解析（预先存在的错误），请参阅 src/testsuite/single/tests/64bit.c 了解详情。

================================================================================
  按版本的变更日志
================================================================================

FluffOS 3.0-alpha7.4
新功能
  * PACKAGE_DB：MYSQL NewDecimal 类型。这在我的系统中需要读取 SELECT 中的 SUM(xxx) 列。（Zoilder）
  * 各种时间跟踪 efun 使用真实世界时间。
  * 驱动现在自动调用 reclaim_objects()。
  * 默认评估器堆栈已增加到 6000。
错误修复：
  * 通过 UDP 端口发送字符串现在不包含尾随 \0。
杂项
  * 将许多内部选项移至 options_internal。
  * 优化函数查找缓存，同时增加默认缓存大小。（CPU 节省！）
  * 新的 tick_event 循环和回调树实现。
  * 调整 backend() 循环，应该没有行为变化。
  * 添加回 32 位编译模式测试。
  * 各种编译修复，虽然不完整，但应该可以在 CYGWIN64 和使用 gcc 4.8 的 freebsd 下编译。

FluffOS 3.0-alpha7.3
  * 用户命令执行现在集成到事件循环中。（至少减少 10% CPU 并改善用户之间的公平性）。
  * 修复 unique_mapping() 在回调返回新对象/数组等时的崩溃。
  * 修复不支持的 TELNET 环境协商时的内存损坏问题。
  * 修复读取 0 长度文件时的内存损坏。
  * 在自动测试中恢复 USE_ICONV。
  * 将许多选项移至 options_internal.h，所有 local_options 覆盖仍然有效。edit_source 将打印出 local_options 包含的 "extra" 定义。这为未来减少 options.h 的复杂性铺平道路。
  * ALLOW_INHERIT_AFTER_FUNCTION 现在默认为真，不再崩溃。
  * 以前如果用户对象被销毁，缓冲区中的消息会丢失。现在驱动将在终止连接之前正确发送它们。

FluffOS 3.0-alpha7.2
  * unique_mapping() 在回调返回非共享字符串时的崩溃。
  * 将一些过时的文档移至 /doc/archive。
  * EFUN/APPLY 文档中的一些格式更改。

FluffOS 3.0-alpha7.1
  * disable_commands() 更改已恢复，未经深思熟虑。
  * enable_commands() 现在接受一个 int 而不是（见 3.0 vs 2.0）

FluffOS 3.0-alpha7
  * 在配置期间检查 C++11 能力。
  * 新的 LPC 预定义 __CXXFLAGS__。
  * 修复 TCP_NODELAY 与 MCCP 的 cmud/zmud 问题。
  * 新的调试宏 "-dadd_action" 以显示 add_action 相关日志。
  * disable_commands() 现在接受一个 int 参数。（见上文）。

FluffOS 3.0-alpha6.4
  * 修复 efun present() 对于以数字结尾的对象 ID 的问题，添加测试。

FluffOS 3.0-alpha6.3
  * 修复 PACKAGE_CRYPTO 的内存损坏问题，添加测试。

FluffOS 3.0-alpha6.2

  * 修复 store_variable() 的内存损坏问题，添加测试。
  * 默认启用用户套接字和 lpc 套接字的 TCP_NODELAY。

FluffOS 3.0-alpha6.1

主要内容：
  * Libevent 2.0 集成：用户和 lpc 套接字使用 epoll()！
  * 异步 DNS：驱动将直接进行异步 DNS，addr_server 支持现在已过时并删除。
  * 所有 *.c 文件已更改为 *.cc。所有 *_spec.c 文件已更改为 *.spec。（如果您维护树外补丁，请注意！）
  * DEBUG_MACRO 现在始终为真，该功能正在作为内置行为规范化。您可以运行 "./driver -d<type>" 来显示驱动中的各种调试消息。（或调用 debug.c 中的 efun，控制台支持稍后推出）。当前 <type> 的选择包括：
       * "-dconnections"，显示所有玩家连接相关事件。
       * "-devent"，显示内部事件循环消息。
       * "-ddns"，显示异步 DNS 解析消息。
       * "-dsockets"，显示所有 lpc 套接字相关消息。
       * "-dLPC"，显示当前 LPC 执行信息。
       * "-dLPC_line"，显示当前执行的 LPC 行。
       * "-dfile"，显示一些文件相关消息。
       * 空（即 "-d"），这目前显示连接 + DNS + 套接字消息，以后会改变。
       * 其他调试消息尚未完全分类，如 "-dflag"。
  * 使用 C++11 mt19937_64 作为随机数生成器引擎。

EFUN：
  * PACKAGE_CONTRIB：store_variable | fetch_variable 接受对象作为第 3 个最后参数。（Lonely@NT）

其他：
  * 在测试套件中恢复 "Mud IP" 配置行，并修复套接字相关代码以 IP 无关。
  * 删除所有旧的 win32/windows 代码和过时的 malloc 实现，现在唯一的选择是 SYSMALLOC（首选）、32bitmalloc 和 64bitmalloc。
  * 大多数内部套接字相关代码现在是 IP 无关的。
  * 强制创建 dwarf-2 调试信息格式。
  * 使 new_user_handler() 对 LPC error() 安全。
  * 添加 C++ 11 的配置检查。
  * 允许将 CXXFLAGS 传递给构建 FluffOS。
  * 修复通过 lpc 套接字发送 0 长度消息时的崩溃。
  * 修复 present("1") 中的缓冲区下溢。
  * 删除文档的 text/html 版本，将 nroff 版本移至 /doc。
  * 添加回 "-flto" 编译选项。

FluffOS 3.0-alpha5
    REVERSE_DEFER，defer() 的 fifo 执行

    重写 unique_mapping()，不再有内存泄漏。
    为 unique_mapping 添加测试。
    使 DEBUG 驱动跳过优雅崩溃例程。
    修复 ltrim 错误（zolider），添加更多测试用例。
    修复损坏的 get_usec_clock（time_expression）。
    还删除一些过时的信号代码。
    修复 PCRE、MYSQL、PGSQL、SQLITE3 的构建。

FluffOS 3.0-alpha4
  PACKAGE_TRIM：（zoilder），字符串修剪的 rtrim、ltrim 和 trim。

  FluffOS 已切换到使用 autoconf 进行兼容性检测。
  这导致 edit_source.c 中的几乎一半代码被删除。
  注意：正确的构建方式仍然是先启动 ./build.FluffOS，然后 make。
  其他一般代码质量改进，许多旧的手艺已被删除。
  FluffOS 3.0 将仅支持现代 Linux 发行版。

FluffOS 3.0-alpha3
  FluffOS 已切换到 C++ 语言。
  使用 try/catch 而不是 longjmp。（wodan）
  代码质量改进。
  修复使用 DEBUG 而没有 DEBUGMALLOC_EXTENSIONS 导致内存损坏。

FluffOS 3.0-alpha2

一般：
  基于 wodan 发布的 2.27 重新基准。
  当 local_options 缺失时，构建将提前失败。
  使用 astyle 强制源格式。

错误修复：
  command() efun 将正确返回 eval 成本。
  Crasher 14，在返回数组类型时崩溃。

测试：
  测试现在随机执行。
  debugmalloc 将用魔数值填充内存。

FluffOS 3.0-alpha1

新的编译选项：
  POSIX_TIMERS：更好的时间精度跟踪用于 eval 成本。（voltara@lpmuds.net）
  CALLOUT_LOOP_PROTECTION：保护 call_out(0) 循环。（voltara@lpmuds.net）
  SANE_SORTING：对 "sort_array()" 使用更快的排序实现，但需要 LPC 代码返回一致的结果。

一般：
  构建脚本改进和编译/警告修复。
  现在支持在 32 位环境中构建。
  现在支持在 CYGWIN 下构建。
  修复多个崩溃/内存泄漏。
  文档已移至根目录。
  启用 Travis CI 以自动化每次提交的测试/构建。
  当驱动崩溃时自动打印回溯转储。
  如果核心转储限制为 0，则在启动时打印警告。
  修复没有 PACKAGE_ASYNC 的 db.c 编译。（mactorg@lpmuds.net）
  一般代码质量改进。

包：
  PACKAGE_CRYPTO：构建修复和增强。（voltara@lpmuds.net）
  PACKAGE_SHA1：修复不正确的 sha1() 哈希生成，已通过测试验证。（voltara@lpmuds.net）

测试：
  "make test" 将启动测试套件并报告任何问题。
  DEBUGMALLOC、DEBUGMALLOC_EXTENSIONS 和 CHECK_MEMORY 现在工作。
  添加了广泛的 64 位运行时测试。
  Switch 操作符测试。
  基准测试器和自动崩溃器改进。

LPC：
  LPC 运行时现在严格为 64 位，一切都应符合。
  MIN_INT、MAX_FLOAT、MIN_FLOAT 预定义。
  EFUN 的最大数量已从 256 提高到 65535。

已知问题：
  "-MAX_INT" 在 LPC 中未正确解析（旧错误），请参阅 src/testsuite/single/tests/64bit.c 了解详情。
  unique_mapping() EFUN 将泄漏内存。
  测试套件中的崩溃需要改进。
