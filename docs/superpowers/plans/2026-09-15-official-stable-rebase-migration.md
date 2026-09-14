# FluffOS 官方稳定版重基线与本地扩展模块化迁移计划（仅本地实施）

> 本计划用于把 `lude2001/fluffos` 从当前独立分叉迁移到官方稳定版本基线。实施时必须按阶段提交和验证，不得把当前 `master` 整体 merge、rebase 或覆盖到新基线上。

> **实施边界：本次迁移完全在本地进行。** 不连接、不修改、不重启线上服务器，不发起生产部署，也不操作线上玩家数据。完成结果是一条通过本地验证的候选分支，不代表已经上线。

**目标：** 以官方 `v2026.0901.0`（提交 `7af5c3fffe2505c7cb764951bed863b16eee471b`）为新的本地候选基线，只移植经确认仍有价值的本地独有能力，并把这些能力整理为可重复移植的编译期扩展、独立工具和少量稳定核心钩子。

**当前基线：** 本地 `master` 为 `98cc9b42b6f7f1630189b4b968ed708ff41f2203`，最近本地标签为 `v2026.07.15`。相对官方 `v2026.0901.0`，GitHub 比较结果为本地独有 130 个提交、缺少官方 311 个提交；这些数字包含回移补丁、测试、文档和构建调整，不能直接当作独有功能数量。

**当前生产构建基线：** `lude2001/fluffos` 的 GitHub Actions `Release Artifacts` 运行 `34217597099` 已在提交 `98cc9b42b6f7f1630189b4b968ed708ff41f2203` 成功生成 `fluffos-linux-static-production`。该工作流使用 `STATIC=ON`、`MARCH_NATIVE=OFF`，启用 MySQL/SQLite，运行 C++ 测试、LPC testsuite、启动 smoke 和静态链接检查，并生成 tarball SHA-256。它可以作为当前生产 driver 的可重建来源，无需访问线上服务器。

**目标架构：** 官方稳定 tag 保持为干净基线；本地能力按主题提交叠加。优先使用 FluffOS 现有 package/`.spec` 自动发现机制。需要访问 VM 或编译器内部状态的能力采用编译期扩展和稳定钩子，不建立依赖内部 C++ 类型 ABI 的运行时 DLL 插件。

**仓库边界：** 官方 `fluffos/fluffos` 只读；不得新增 `upstream` remote，不得向官方仓库创建或修改 PR、分支或 release。所有可写操作仅限 `origin` 对应的 `lude2001/fluffos`。

## 风险审查结论

原计划方向成立，但第一版把风险控制放得太靠后。江湖英杰传不是“将来可能使用”这些扩展的测试 mudlib，而是当前生产消费者，因此迁移顺序必须由现网契约反向驱动。

当前已确认的高风险事实：

- 江湖英杰传源码约有 100 处非测试 `json_encode()` 调用、30 处非测试 `json_decode()` 调用；JSON 参与登录、客户端协议、autoload、配置和业务数据文件，属于 P0 运行契约。
- HTTP 请求/响应增量解析 efun 被现有 LPC HTTP 服务直接调用，影响管理、部署和对外集成入口，不是可延后补齐的开发辅助功能。
- mudlib 有多条 `save_object()`、`restore_object()`、`save_variable()`、`restore_variable()` 链路。只证明新 driver 能读旧存档不够；若新 driver 写出旧 driver 无法恢复的数据，生产回退会失效。
- 候选实例若直接复用生产数据库、玩家存档、支付/QQ/外部 HTTP 或相同监听端口，会产生双写、重复副作用或真实玩家误入，不能作为测试方式。
- 当前 `lpccp` 的目录重载存在已登记的 driver 崩溃缺陷；它不能用于生产迁移或整库验收，只能在隔离开发实例中按安全边界使用。
- 本地 `build/dist` 中存在旧 Windows 构建，可作为本地行为对照；源码回退以当前 Git 提交和 legacy tag 为准。

因此，本计划采用以下修正：先用 Git 保留当前版本，再建立必要的兼容清单；先在旧基线整理扩展边界，再在官方基线移植；最后只在本地使用江湖英杰传 mudlib 和测试角色存档验证。

---

## 一、不可变原则

- [ ] 不在当前 `master` 上直接合并官方 `master`。
- [ ] 不以官方开发分支作为生产基线；本轮固定使用正式 release `v2026.0901.0`。
- [ ] 迁移期间固定该 release，不因官方又发布新版本而中途追赶；后续版本另开差异审计。
- [ ] 不搬运自动生成文件、旧 third-party vendor 差异和已被官方实现覆盖的补丁。
- [ ] 保留本仓库现有轻量 CI/CD 的职责、触发方式和产物结构；不得用官方仓库更重的 workflow 整体覆盖。
- [ ] 不把当前 130 个本地提交逐个机械 cherry-pick。
- [ ] 每项本地能力必须有独立边界、独立测试和独立提交。
- [ ] 当前 `master` 在切换前必须保留不可变 legacy tag 和可恢复引用。
- [ ] 未通过完整 Windows、Linux、LPC testsuite 和实际 mudlib 验证前，不替换 `master`。
- [ ] 不以“构建成功”代替运行时、协议、TLS、数据库和玩家可达性验证。
- [ ] 候选 driver 不直接读取或写入唯一一份生产存档，不与旧 driver 并发写同一数据库或数据目录。
- [ ] 不把 TLS 策略调整、数据库配置改造和 driver 重基线合成一次不可拆分发布。
- [ ] 本轮禁止任何线上服务器操作，包括远程命令、上传、部署、重启、热编译、线上探针和线上数据读取/写入。

### 能力分层

迁移前先把本地能力按生产必要性分层，避免把开发工具和线上运行时绑成一个不可裁剪整体：

1. **生产硬依赖：** native JSON、HTTP helper/parser、现有 DB/TLS/WebSocket 行为以及江湖英杰传实际调用到的 efun/apply/配置语义。
2. **运行时可观测性：** mapping 归因和额外统计；确认江湖 mudlib 或本地工具是否实际消费后再决定是否保留。
3. **开发工具：** `lpccp`、运行时编译服务、`dev_test` 调用入口、Windows `lpcprj` 和安装器。必须保留开发能力，但默认不得扩大生产攻击面。
4. **官方已实现能力：** `recompile_object()`、`sys_reload_tls()`、`get_os_env()`、`set_os_env()`、`request_clean_up()`、`set_clean_up()`、`to_buffer()` 等先以官方实现为准，只做兼容测试，不再作为本地独有代码搬运。

---

## 二、本地能力清单与迁移决策

### A. 必须保留并重构

#### 1. 运行时编译服务、`lpccp` 与 `dev_test`

当前组成：

- `src/compile_service.*`
- `src/compile_service_protocol.h`
- `src/runtime_compile_request.*`
- `src/runtime_dev_test_request.*`
- `src/main_lpccp.cc`
- `docs/cli/lpccp.md`

迁移策略：

- [ ] 将其归为开发工具能力，不作为生产 driver 的默认必启服务。
- [ ] 先冻结当前 CLI、JSON 响应和 canonical fixture；保留 `dev_test` 只是兼容已有工具协议，不代表本次迁移要改动 mudlib 测试体系。
- [x] 已复核官方新版 `lpcc` 的 JSON/batch 和诊断接口；新基线保留官方离线工具，本地扩展只承担运行中 VM 请求。
- [ ] 保留运行中 VM reload、目录编译和 `dev_test()` 等官方 `lpcc` 无法替代的能力。
- [x] 旧基线已把 named-pipe 接入、协议、排队、runtime 请求适配和客户端代码聚合到 `src/extensions/compile_service/`，服务实现不再混入 `libdriver` source 列表。
- [ ] 核心 driver 只保留 VM started、tick、shutdown 三个扩展钩子。
- [ ] 编译诊断通过适配层连接官方新版编译器，不让协议层直接依赖编译器内部布局。
- [ ] 编译服务必须显式启用或限制到本机当前用户；不得继续默认授予 Everyone 全权限。
- [ ] 在隔离实例修复并验证“目录重载错误导致响应 JSON 为空并终止 driver”的已知缺陷；修复前禁止用目录重载作为迁移验收或生产操作。

旧基线验证：`build.cmd` 完整通过，`fluffos_compile_service` 独立静态库与 driver 成功链接；`lpccp.exe` 已不再链接 `${FLUFFOS_LINK}`，`objdump` 只显示 Windows 系统 DLL。协议、客户端、队列、runtime adapter 和编译器回归共 27 项通过。Windows CI 已排除的 `CompileServiceTransport.ConcurrentPipeClientsCanBothReceiveResponses` 仍稳定失败于首个客户端 `win32=2`，作为既有缺陷保留，不在这次机械抽离中伪装成已解决。

#### 2. 原生 JSON efun

当前能力：`json_decode()`、`json_encode()`、`json_format()`。

迁移策略：

- [x] 保持为本地独有的 `src/packages/json/` drop-in package；官方当前没有同名 native package，无需为了命名空间改动现有兼容标记。
- [x] efun 公共名称保持不变，避免 mudlib 改动。
- [x] 将 JSON 标记为生产 P0 依赖；在它通过兼容契约前，不开始真实 mudlib 候选启动。
- [x] package 自己声明 CMake option，避免修改官方集中 option 列表。
- [ ] 保留循环引用、非字符串 mapping key、undefined/null、整数边界和格式化测试。
- [ ] 固定当前实现的 mapping 字段顺序、UTF-8/转义、数字类型与边界、非法输入错误、循环引用转 null、非字符串 key 忽略和不支持类型转 null 等行为。
- [ ] 从江湖英杰传的登录、首页、背包、战斗、autoload 和配置文件链路生成脱敏 canonical fixtures，逐字节比较新旧 driver 输出，并验证 Flutter 消费端既有 fixture。
- [ ] 对照官方 `testsuite/std/json.lpc`，明确原生 efun 与 LPC 标准库的优先级和兼容语义；不得未经证据把 native efun 替换为官方 LPC `std/json.lpc`。

#### 3. HTTP helper 与增量解析 efun

当前能力包括 URL 编解码、query/form 解码、HTTP 响应构造、请求与响应增量解析器。

迁移策略：

- [x] 已在旧基线上从 sockets package 的 `sockets.cc`、`sockets.spec` 和 source 列表中解耦。
- [x] 已独立为 `src/packages/lude_http/`，拥有自己的 CMake option、`.spec`、实现和既有测试。
- [x] 保持现有 11 个 efun 名称和返回 mapping 结构；Windows 规范构建及 helper、request parser、response parser 三组定向测试通过。
- [ ] 为请求大小、header/body 上限、parser handle 生命周期和异常清理补明确边界。
- [ ] 使用当前 LPC HTTP 服务的真实分片方式固定请求/响应 parser fixtures，覆盖 partial header、partial body、chunked、Content-Length、连接关闭和错误输入。
- [ ] 候选环境禁用支付、QQ 消息、部署控制和外部 HTTP 写操作；验证解析器时使用本地假服务，不调用真实第三方。

#### 4. Windows 构建、发布目录、`lpcprj` 和安装器

迁移策略：

- [x] 旧基线已确认并保留根目录 `build.cmd` 作为 Windows 规范入口。
- [x] 旧基线已确认 `build/dist` 是唯一支持的本地 Windows 可运行输出目录。
- [x] 打包逻辑位于 `scripts/`、`packaging/windows/` 和 release workflow，没有进入 VM。
- [x] `lpcprj` 是只依赖 C++ 标准库与 Windows API 的独立 executable，不链接 driver 内部实现；现有 MariaDB 本地兼容环境变量保持不变。
- [x] `stage-driver-dist.ps1` 通过 `objdump` 递归发现非系统 DLL，安装镜像再从 dist 自动复制运行时 DLL；没有维护第三方 DLL 固定复制清单。

旧基线验证：`build.cmd` 完整成功并生成 dist、install image 与 installer；`test-install-image-layout.ps1`、`test-run-driver-relative-config.ps1`、`test-lpcprj-relative-config.ps1`、`test-windows-installer-config.ps1` 和临时目录静默安装/用户 PATH 回滚测试均通过。

#### 5. 本仓库轻量 CI/CD 与静态生产 driver

必须保留的现有工作流：

- `.github/workflows/ci-ubuntu.yml`
- `.github/workflows/ci-macos.yml`
- `.github/workflows/ci-windows.yml`
- `.github/workflows/release-artifacts.yml`

迁移策略：

- [ ] 不导入官方 CI、Docker publish、CodeQL、Coverity、文档站和多余发布矩阵。
- [ ] 保持 Ubuntu、macOS、Windows 三个平台的轻量 configure/build/test 流程大体不变。
- [ ] 保持 `Release Artifacts` 可按 `windows`、`linux`、`linux-static`、`macos` 或 `all` 手动构建。
- [ ] 保持静态生产 driver 的关键参数：Release、`STATIC=ON`、`MARCH_NATIVE=OFF`、MySQL、SQLite 和默认 DB handle。
- [ ] 保持静态链接的 `file`、`readelf`、`ldd` 三重检查，以及 tarball `.sha256` 输出。
- [ ] 保持 Windows installer/runtime ZIP、普通 Linux runtime、静态 Linux runtime和 macOS runtime 的既有产物用途。
- [ ] 允许为适配官方新基线调整 runner、依赖包名、CMake 参数或测试排除项，但不得未经证据扩大成官方同等规模的工作流。
- [ ] 迁移分支在本地验证后，如需使用 GitHub CI，可在用户明确授权后只推送到 `lude2001/fluffos`；运行 CI 不等于部署线上服务器。

#### 6. mapping 存活数量和创建程序归因

迁移策略：

- [ ] 统计实现保留在独立扩展或 `mudlib_stats` 扩展中。
- [ ] 官方 `mapping.cc` 只允许保留 allocation/deallocation 的可选 instrumentation hook。
- [ ] hook 必须在 package 关闭时编译为空操作。
- [ ] 验证 allocate、copy、free、异常展开和 driver 内部无 `current_object` 分配的计数平衡。
- [ ] 先查明江湖 mudlib 或本地工具是否消费这些统计；若没有，它属于可选的可观测性增强。

### B. 必须重新审计，禁止直接搬运

- [ ] `avoid false inherited prototype warnings` 编译器补丁。
- [ ] mapping ref type-confusion 与 `evaluate()` 生成限制补丁。
- [ ] DB CMake 冲突检测和跨平台构建补丁。
- [ ] Bison 生成路径正规化补丁。
- [ ] `memory_summary()` 与 JSON mapping 参数修复。
- [ ] 所有手工回移的官方安全、VM、parser、buffer、hot-reload 和 `recompile_object()` 补丁。

官方 `v2026.0901.0` 已包含 `recompile_object()`、`sys_reload_tls()`、`get_os_env()`、`set_os_env()`、`request_clean_up()`、`set_clean_up()` 和 `to_buffer()` 等当前 mudlib 可能调用的能力。这些项目先建立行为兼容用例，再使用官方实现；只有出现可复现的不兼容时才增加最小适配层。

每项处理规则：先在官方 `v2026.0901.0` 上运行原回归测试；测试已通过则删除本地补丁，只保留必要测试。只有能在新基线复现缺陷时，才重写最小修复。

### C. 不进入新运行时基线

- [ ] 官方额外 CI/release workflow 及与本仓库轻量流程无关的 workflow 历史差异。
- [ ] 当前分支的第三方源码快照差异。
- [ ] 旧 compiler layout 专用生成文件和适配代码。
- [ ] 已由官方提供的回移功能实现。
- [ ] 过期设计分支、历史 PR 分支和纯临时 checkpoint。

---

## 三、MariaDB 本地连接过渡方案

当前 `lpcprj` 无条件设置 `MARIADB_TLS_DISABLE_PEER_VERIFICATION=1`。它确实关系到本地连接远端 PolarDB 的可达性，但目前没有完成开启 peer verification 的 A/B 实连证据。因此，不能在 driver 重基线时顺手删除，也不能把它扩散到新的生产启动路径。

### 阶段一：重基线只保持现状，不合并安全策略变更

- [ ] 冻结当前 `lpcprj` 行为和 Connector/C 版本，证明候选版本在相同条件下仍能连接；这一步不改变现有开发连接策略。
- [ ] 本轮只验证本地 `lpcprj` 和 `config/config.dev` 的连接行为，不检查线上启动环境。
- [ ] 变量仅允许保留在既有本地开发启动边界，不添加到 Linux、服务端或通用 driver 默认环境。
- [ ] 明确记录：该变量只要存在就会关闭 peer verification，设置为 `0` 也不是安全恢复。
- [ ] driver 重基线验收期间不同时发布 TLS 策略改变，以便数据库连接回归能够单独定位和回退。

### 阶段二：重基线稳定后单独建立正常证书验证

- [ ] 从 PolarDB 控制台确认 SSL 状态并下载官方 CA chain。
- [ ] 为 MySQL adapter 增加 CA 路径、强制 TLS 和 peer/hostname verification 配置。
- [ ] 在 `mysql_real_connect()` 前设置 Connector/C TLS options。
- [ ] 使用数据库原始域名连接，不以 IP 或不匹配证书的本地别名代替。
- [ ] 分别验证直接运行 `driver.exe` 与通过 `lpcprj` 启动。
- [ ] 只读执行 `SELECT 1`，并核对连接实际使用的 TLS cipher。
- [ ] 单独提交、构建和发布这一安全调整；验证成功后再删除本地兼容降级选项。

---

## 四、分支和提交策略

### Task 0：固定迁移起点

- [x] 已确认当前 `master == origin/master == 98cc9b42b6f7f1630189b4b968ed708ff41f2203`，除迁移计划外没有其他未提交改动。
- [x] 已为该提交创建本地 annotated tag `legacy/pre-official-rebase-2026-09-15`，保证任何时候都能从 Git 回到迁移前源码；该 tag 尚未推送。
- [x] 已记录 GitHub Actions `Release Artifacts` 运行 `34217597099`：它已经从同一提交成功生成 `fluffos-linux-static-production`，因此不需要访问线上服务器取得旧 driver。
- [x] 已确认 Actions artifact 过期不影响源码回退；必要时可从 legacy tag 重新运行现有静态构建工作流。现阶段不额外下载 artifact。

### Task 1：确认旧版本本地可用

- [x] 使用当前 `master` 和现有 `build.cmd` 完成本地构建；`build/dist` 分阶段成功，`driver.exe` SHA-256 为 `CAA877C045C4476EAC7D11FFAEE53F8BEE244A0981518E88823F57CDCFC9115A`。
- [x] 使用江湖英杰传本地 `config/config.dev` 的本地验证副本启动当前 driver。因已有本地 driver 占用 `4000/8888`，验证副本只改为 `14000/18888` 并使用绝对 mudlib 路径；全部 preload 完成、两个端口监听成功并进入 `Initializations complete`。原 `config.dev` 和已有本地 driver 均未修改、未停止。
- [x] 使用账号池中的本地测试角色 `gameteststd1` 完成真实回环登录和 `look`：结构化登录结果与步骤结果均为 `ok: true`，角色档、背包和房间数据恢复正常。未造状态、未执行写入命令、未清理存档；正常登录流程按现有语义更新了该测试账号的本地登录/角色存档时间。

### Task 2：确定独有功能清单

- [x] 已比较当前分叉与官方 `v2026.0901.0`，把差异分为：本地独有功能、官方已包含功能、构建/文档差异和不需要迁移的历史补丁。
- [x] 已用江湖英杰传源码确认实际调用到的独有 efun 和工具，重点是 native JSON、HTTP helper/parser、编译工具、Windows 打包和 mapping 统计。
- [x] 已在 [`2026-09-15-official-stable-unique-capability-inventory.md`](2026-09-15-official-stable-unique-capability-inventory.md) 写明每项功能的“保留、由官方替代或不迁移”决定；Task 3 以该清单为抽离边界。

### Task 3：在旧基线上抽离扩展边界

- [x] 已从当前 `master` 创建短期本地分支 `codex/legacy-extension-extraction`。
- [x] 已完成行为保持的目录移动、适配层和最小钩子收敛；没有改变协议、TLS、存档或运行语义。
- [x] JSON、HTTP、Windows 交付边界、轻量 CI 契约和 compile service 分别形成独立提交；JSON/HTTP/Windows/compile service 均已完成对应构建或契约测试。
- [x] 抽离分支固定在 `a3cbf2bf5815d4fa269712fb4e9c37a2773d4b56`，只作为可重放、可审查的扩展切片保留；未部署、未推送，迁移完成前不删除。

### Task 4：建立官方稳定基线迁移分支

- [x] 已通过官方仓库 URL 只读获取 tag `v2026.0901.0`，没有新增 remote；仓库仍只有 `origin` 指向 `lude2001/fluffos`。
- [x] 已验证 annotated tag 及其 peeled commit，官方稳定基线为 `7af5c3fffe2505c7cb764951bed863b16eee471b`。
- [x] 已从该提交创建 `codex/rebase-v2026.0901.0`。
- [x] 已在未加本地功能前用官方 Windows CI 等价参数完成 RelWithDebInfo 构建和安装；340 项非 testsuite 测试全部通过。
- [x] 官方 testsuite 在原配置下因现有本地 driver 占用 `4000` 至 `4003` 而无法收尾；不停止现有进程，复制测试目录并把配置及两项端口断言同步改到 `24000` 至 `24003` 后，711 个 LPC 文件、10655 项检查全部通过并输出 `Checks succeeded.`。
- [x] 已记录纯官方基线现象：禁用 MySQL、启用 SQLite 时配置阶段仍打印 `FATALPACKAGE_DB_DEFAULT_DB is not valid!` 但返回成功；MinGW/GCC 对官方及 vendored 源码产生多项既有警告。它们不是本地迁移回归，后续候选以相同工具链比较。

### Task 5：按生产依赖顺序移植

建议提交顺序：

1. repository policy 和本地文档边界
2. 保留并最小适配本仓库 CI/CD
3. 本地构建 profile、制品清单和可重复 staging
4. native JSON package
5. HTTP helper/parser package
6. 江湖英杰传实际依赖的最小官方行为适配
7. mapping instrumentation（确认有消费者时）
8. Windows dist/staging、安装器与 `lpcprj`
9. compile-service core hooks
10. compile service、协议和 `lpccp`
11. 经新基线复现后仍需要的最小兼容补丁

每个主题必须先有测试或 fixture，再移植实现；不得把多个能力压进一个巨型提交。

当前迁移进度：

- [x] repository policy、迁移文档和 upstream tracker 已落到新基线。
- [x] 已用本仓库四个轻量 workflow 替换官方重型 workflow，并把测试门禁适配为官方 CTest labels。
- [x] native JSON 已作为独立 package 接入；新增的 native efun 契约测试 32 项通过，官方 LPC `std/json.lpc` 原有 178 项测试也继续通过。
- [x] HTTP helper、request parser 和 response parser 已作为独立 `lude_http` package 接入；三份契约测试共 90 项检查通过，官方 sockets package 未修改。
- [ ] 江湖兼容、Windows 交付层和 compile service 尚待依次接入与验证。

### Task 6：本地候选环境验证

- [ ] 使用江湖英杰传本地工作树和 `config/config.dev`；需要写存档的验证只使用本地测试角色。
- [ ] 运行旧、新 driver 时避免复用相同端口、日志和 IPC 名称；同一份 config 默认一次只运行一个实例。
- [ ] 不触发支付、QQ 消息、邮件、部署控制、公告等外部写操作。
- [ ] 完整冷启动 master、simul_efun 和 preload，编译真实 mudlib；不得用对生产目录执行批量 `lpccp --reload-loaded` 代替冷启动。
- [ ] 用同一组输入分别运行旧 driver 与候选 driver，比较协议字节、状态快照、日志错误和关键性能指标。
- [ ] 复制一个本地测试角色存档进行旧→新恢复和保存验证；原测试存档保留不动。
- [ ] 进行长时间 soak，覆盖心跳、call_out、异步 DB/IO、对象 swap/reload、WebSocket/TLS 和内存趋势。
- [ ] 记录候选 driver 和运行时文件 SHA-256，作为本地验收结果。

### Task 7：本地收尾

- [ ] 所有本地验收通过后，生成旧基线与候选基线的差异、验证结果和未验证项报告。
- [ ] 更新 `docs/superpowers/upstream-merge-tracker.md`，记录官方快照、保留能力、删除补丁和验证结果。
- [ ] 候选分支保持独立；本轮不合并回 `master`、不推送、不发布、不部署。
- [ ] 保留 legacy tag 和本地旧构建。将来若决定上线，另写简短上线清单并重新取得授权。

---

## 五、验证矩阵

### 构建

- [ ] Windows：`build.cmd` 完成，所有支持工件位于 `build/dist`。
- [ ] Linux：完成 Release、`MARCH_NATIVE=OFF` 的可移植构建。
- [ ] GitHub `Release Artifacts` 的 `linux-static` 目标能够生成 `fluffos-linux-static-production`，并通过静态链接检查和 SHA-256 输出。
- [ ] 官方启用的 sanitizer/单元测试配置至少完成一轮。
- [ ] `git diff --check` 无错误。
- [ ] 每次候选构建记录源码提交、工具链、flags、依赖版本、制品列表和 SHA-256；禁止以文件时间或文件名认定版本。

### Driver 与 LPC

- [ ] C++ 单元测试通过。
- [ ] 官方 LPC testsuite 全量运行；普通平台 CI 的既有容错策略可以保留，但 `Release Artifacts` 的静态生产构建必须严格通过。
- [ ] 当前游戏 mudlib 在隔离冷启动中完成 master、simul_efun、preload 和可达业务 owner 编译。
- [ ] master、simul_efun、继承链、clone 和 hot reload 定向用例通过。
- [ ] 新旧 driver 对关键 LPC 语义的差异得到解释和批准。
- [ ] `save_object/restore_object`、`save_variable/restore_variable` 完成旧→新和新→旧双向兼容测试。

### 本地独有能力

- [ ] JSON canonical fixture 在旧/新 driver 间字节和语义一致，覆盖真实客户端协议、autoload 和配置数据。
- [ ] HTTP 请求/响应 parser 的增量、边界和错误用例通过。
- [ ] mapping 统计分配/释放后回到基线。
- [ ] `lpccp` compile、reload-loaded、compile-only、fresh-required 和协议兼容通过；目录模式只能在崩溃缺陷修复后于隔离实例验证，禁止对生产目录运行。
- [ ] 并发请求保持 FIFO 串行 VM 执行，关闭和超时路径无悬挂任务。

### 江湖英杰传本地集成

- [ ] Windows 安装器、runtime ZIP 和相对路径启动验证通过。
- [ ] TLS/WebSocket 实际握手通过，不存在证书校验降级。
- [ ] 本地 PolarDB 正常证书验证连接通过。
- [ ] 本地开发数据库连接在现有开发配置下通过；不连接或操作线上游戏服务器。
- [ ] 使用本地测试角色完成登录、重连、保存恢复和核心流程检查。
- [ ] 候选 soak 期间无新增 driver 崩溃、runtime error、存档错误、协议错误、数据库错误或持续内存增长。
- [ ] 整个验证过程没有触发远程部署、线上热编译或线上玩家数据变更。

---

## 六、回退方案

- [ ] legacy tag 指向当前生产前基线，禁止移动。
- [ ] 保留旧 `build/dist` 或可重新构建旧版本的明确命令；新旧 driver 放在不同本地目录。
- [ ] 候选失败时直接停止候选实例并切回 legacy tag，不修改线上环境。
- [ ] 本地存档测试只操作副本，失败时删除测试副本即可，原测试角色存档不受影响。

---

## 七、完成标准

迁移只有同时满足以下条件才算完成：

- 官方稳定 tag 来源和 commit 已固定并可复核。
- 当前 FluffOS 和江湖英杰传本地 Git 基线已记录并可复核。
- 本地独有能力均有明确归属：保留、重写、由官方替代或删除。
- 江湖英杰传的静态、动态和外部消费者依赖已形成机器可比较契约。
- 官方核心文件中的本地改动收敛到少量、有测试的扩展钩子。
- `build/dist`、安装器、`lpccp`、`lpcprj`、JSON、HTTP 和 mapping 统计全部通过验证。
- MariaDB 本地连接不再依赖默认关闭证书校验。
- 完整 testsuite、实际 mudlib 和本地测试角色 smoke test 均有证据。
- 候选可以读取并保存本地测试角色存档，原存档副本未被覆盖。
- 候选制品经过本地验证并记录 SHA-256。
- legacy 版本可恢复，迁移期间没有删除唯一回退引用。
- 没有对线上服务器、线上进程或线上玩家数据执行任何操作。

## 八、开始代码迁移前的条件

开始建立官方基线分支前只要求三件事：

- [ ] 当前 FluffOS `master` 已建立 legacy tag，可以从 Git 恢复。
- [ ] 已列出江湖英杰传真正依赖的本地独有能力，至少包括 JSON 和 HTTP parser。
- [ ] 当前 `master` 保留 legacy tag，新迁移在独立分支进行，不影响现有生产维护。

本地存档验证和候选运行属于迁移后期的本地验收。生产切换不在本计划范围内；将来如需上线，必须作为新的独立任务重新确认。
