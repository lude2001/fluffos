# FluffOS 官方稳定版重基线与本地扩展模块化迁移计划（仅本地实施）

> 本计划用于把 `lude2001/fluffos` 从当前独立分叉迁移到官方稳定版本基线。实施时必须按阶段提交和验证，不得把当前 `master` 整体 merge、rebase 或覆盖到新基线上。

> **实施边界：本次迁移完全在本地进行。** 不连接、不修改、不重启线上服务器，不发起生产部署，也不操作线上玩家数据。完成结果是一条通过本地验证的候选分支，不代表已经上线。

**目标：** 以官方 `v2026.0901.0`（提交 `7af5c3fffe2505c7cb764951bed863b16eee471b`）为新的本地候选基线，只移植经确认仍有价值的本地独有能力，并把这些能力整理为可重复移植的编译期扩展、独立工具和少量稳定核心钩子。

**迁移前基线：** 本地 `master` 为 `98cc9b42b6f7f1630189b4b968ed708ff41f2203`，最近本地标签为 `v2026.07.15`。相对官方 `v2026.0901.0`，GitHub 比较结果为本地独有 130 个提交、缺少官方 311 个提交；这些数字包含回移补丁、测试、文档和构建调整，不能直接当作独有功能数量。

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

- [x] 没有在当前 `master` 上直接合并官方 `master`。
- [x] 没有使用官方开发分支；本轮固定使用正式 release `v2026.0901.0`。
- [x] 迁移期间固定该 release，没有因官方后续变化中途追赶。
- [x] 没有搬运自动生成文件、旧 third-party vendor 差异和已被官方实现覆盖的补丁。
- [x] 保留了本仓库现有轻量 CI/CD 的职责、触发方式和产物结构，没有导入官方重型 workflow。
- [x] 没有把当前 130 个本地提交逐个机械 cherry-pick。
- [x] 各项本地能力均按独立边界、测试和提交迁移。
- [x] 当前 `master` 已由不可变 legacy tag 和旧 dist 保留。
- [x] 本地验证完成后，以 legacy tag 保留旧 `master`，再把本地 `master` 指向候选提交；没有创建会把旧版 130 个提交重新引入新基线的普通 merge commit。`origin/master` 尚未更新，Linux/GitHub CI 仍属于将来准备推送或发布时的独立门禁。
- [x] 验收包含构建、协议、完整 testsuite、实际 mudlib、测试角色与双向存档，不以构建成功代替运行验证。
- [x] 候选 driver 只使用隔离存档副本，没有与旧 driver 并发写同一数据库或数据目录。
- [x] TLS 策略调整与数据库配置改造没有并入 driver 重基线。
- [x] 全程没有执行任何线上服务器操作。

### 能力分层

迁移前先把本地能力按生产必要性分层，避免把开发工具和线上运行时绑成一个不可裁剪整体：

1. **生产硬依赖：** native JSON、HTTP helper/parser、现有 DB/TLS/WebSocket 行为以及江湖英杰传实际调用到的 efun/apply/配置语义。
2. **运行时可观测性：** mapping 归因和额外统计；确认江湖 mudlib 或本地工具是否实际消费后再决定是否保留。
3. **开发工具：** `lpccp`、运行时编译服务、Windows `lpcprj` 和安装器。必须保留开发能力，但默认不得扩大生产攻击面。
4. **官方已实现能力：** `recompile_object()`、`sys_reload_tls()`、`get_os_env()`、`set_os_env()`、`request_clean_up()`、`set_clean_up()`、`to_buffer()` 等先以官方实现为准，只做兼容测试，不再作为本地独有代码搬运。

---

## 二、本地能力清单与迁移决策

### A. 必须保留并重构

#### 1. 运行时编译服务与 `lpccp`

当前组成：

- `src/compile_service.*`
- `src/compile_service_protocol.h`
- `src/runtime_compile_request.*`
- `src/main_lpccp.cc`
- `docs/cli/lpccp.md`

迁移策略：

- [x] 已将其归为开发工具能力；CMake 默认关闭，仅本仓库 Windows 开发构建与 Windows CI 显式启用。
- [x] 当前 CLI、JSON 响应和协议测试保持兼容；mudlib 的测试体系不属于本次重构范围。
- [x] 已复核官方新版 `lpcc` 的 JSON/batch 和诊断接口；新基线保留官方离线工具，本地扩展只承担运行中 VM 请求。
- [x] 保留运行中 VM compile/reload 和目录编译等官方离线 `lpcc` 无法替代的能力。
- [x] 旧基线已把 named-pipe 接入、协议、排队、runtime 请求适配和客户端代码聚合到 `src/extensions/compile_service/`，服务实现不再混入 `libdriver` source 列表。
- [x] 核心 driver 只保留 start、tick、shutdown、编译输出和运行错误采集等少量条件编译钩子；关闭 option 时不链接扩展。
- [x] 编译诊断通过 runtime adapter 连接官方新版编译器，协议层不依赖编译器内部布局。
- [x] named pipe 使用受保护 DACL，只允许当前进程用户、SYSTEM 和 Administrators；不再授予 Everyone 或 Authenticated Users。
- [x] 本轮没有使用目录重载作为迁移验收或生产操作；该路径的进一步强化属于后续独立开发工具任务。

旧基线验证：`build.cmd` 完整通过，`fluffos_compile_service` 独立静态库与 driver 成功链接；`lpccp.exe` 已不再链接 `${FLUFFOS_LINK}`，`objdump` 只显示 Windows 系统 DLL。协议、客户端、队列、runtime adapter 和编译器回归共 27 项通过。Windows CI 已排除的 `CompileServiceTransport.ConcurrentPipeClientsCanBothReceiveResponses` 仍稳定失败于首个客户端 `win32=2`，作为既有缺陷保留，不在这次机械抽离中伪装成已解决。

#### 2. 原生 JSON efun

当前能力：`json_decode()`、`json_encode()`、`json_format()`。

迁移策略：

- [x] 保持为本地独有的 `src/packages/json/` drop-in package；官方当前没有同名 native package，无需为了命名空间改动现有兼容标记。
- [x] efun 公共名称保持不变，避免 mudlib 改动。
- [x] 将 JSON 标记为生产 P0 依赖；在它通过兼容契约前，不开始真实 mudlib 候选启动。
- [x] package 自己声明 CMake option，避免修改官方集中 option 列表。
- [x] 同一份 32 项契约覆盖循环引用、非字符串 mapping key、undefined/null、整数边界、格式化和非法输入。
- [x] 已固定 mapping 字段顺序、UTF-8/转义、数字边界、循环引用转 null、非字符串 key 忽略和不支持类型转 null 等现有行为。
- [x] 同一份契约已在旧、新 driver 上逐字节核对；江湖真实登录和 `look` 另覆盖实际消息链。由于协议没有变化，本轮不扩大为 Flutter 跨仓库重构。
- [x] 官方 `testsuite/std/json.lpc` 的 178 项检查继续通过；native efun 保持独立 package，没有被官方 LPC 标准库替换。

#### 3. HTTP helper 与增量解析 efun

当前能力包括 URL 编解码、query/form 解码、HTTP 响应构造、请求与响应增量解析器。

迁移策略：

- [x] 已在旧基线上从 sockets package 的 `sockets.cc`、`sockets.spec` 和 source 列表中解耦。
- [x] 已独立为 `src/packages/lude_http/`，拥有自己的 CMake option、`.spec`、实现和既有测试。
- [x] 保持现有 11 个 efun 名称和返回 mapping 结构；Windows 规范构建及 helper、request parser、response parser 三组定向测试通过。
- [x] 90 项契约已覆盖请求/header/body 边界、parser handle 生命周期和异常清理。
- [x] 请求/响应 parser fixtures 覆盖 partial header、partial body、chunked、Content-Length、连接关闭和错误输入。
- [x] 候选验证没有执行支付、QQ 消息、部署控制或外部 HTTP 写操作，也没有调用真实第三方。

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

- [x] 没有导入官方 CI、Docker publish、CodeQL、Coverity、文档站或多余发布矩阵。
- [x] Ubuntu、macOS、Windows 三个平台的轻量 configure/build/test 流程保持原有职责。
- [x] `Release Artifacts` 仍可按 `windows`、`linux`、`linux-static`、`macos` 或 `all` 手动构建。
- [x] 静态生产 driver 保持 Release、`STATIC=ON`、`MARCH_NATIVE=OFF`、MySQL、SQLite 和默认 DB handle 参数。
- [x] 静态链接仍保留 `file`、`readelf`、`ldd` 三重检查和 tarball `.sha256` 输出。
- [x] Windows installer/runtime ZIP、普通 Linux runtime、静态 Linux runtime 和 macOS runtime 的产物用途保持不变。
- [x] 只做了适配官方新基线所需的参数和测试门禁调整，没有扩张成官方同等规模的工作流。
- [x] 本轮没有推送；将来运行 GitHub CI 仍须用户明确授权，并且不等于部署线上服务器。

#### 6. mapping 存活数量和创建程序归因

迁移策略：

- [x] 已审计江湖 mudlib 和本地工具，没有发现 mapping 创建程序归因统计的消费者。
- [x] 本轮明确不迁移该 instrumentation，候选直接使用官方 `mapping.cc`，不保留 allocation/deallocation hook。
- [x] 因没有迁移 hook，不存在 package 关闭或计数平衡的新增验收项。
- [x] 该能力归为将来可选的可观测性增强，不是生产重基线依赖。

### B. 必须重新审计，禁止直接搬运

- [x] `avoid false inherited prototype warnings` 编译器补丁已重新审计，不直接搬运。
- [x] mapping ref type-confusion 与 `evaluate()` 生成限制补丁已重新审计，不直接搬运。
- [x] DB CMake 冲突检测和跨平台构建补丁已重新审计，以官方实现为准。
- [x] Bison 生成路径正规化补丁已重新审计，没有搬运旧生成文件。
- [x] `memory_summary()` 与 JSON mapping 参数修复已重新审计；只保留独立 native JSON 能力。
- [x] 手工回移的官方安全、VM、parser、buffer、hot-reload 和 `recompile_object()` 补丁均以新基线测试结果决定，没有机械搬运。

官方 `v2026.0901.0` 已包含 `recompile_object()`、`sys_reload_tls()`、`get_os_env()`、`set_os_env()`、`request_clean_up()`、`set_clean_up()` 和 `to_buffer()` 等当前 mudlib 可能调用的能力。这些项目先建立行为兼容用例，再使用官方实现；只有出现可复现的不兼容时才增加最小适配层。

每项处理规则：先在官方 `v2026.0901.0` 上运行原回归测试；测试已通过则删除本地补丁，只保留必要测试。只有能在新基线复现缺陷时，才重写最小修复。

### C. 不进入新运行时基线

- [x] 官方额外 CI/release workflow 及无关 workflow 历史差异没有进入新基线。
- [x] 旧分支的第三方源码快照差异没有进入新基线。
- [x] 旧 compiler layout 专用生成文件和适配代码没有进入新基线。
- [x] 已由官方提供的回移功能实现没有重复迁移。
- [x] 过期设计分支、历史 PR 分支和纯临时 checkpoint 没有进入新运行时基线。

---

## 三、MariaDB 本地连接过渡方案

当前 `lpcprj` 无条件设置 `MARIADB_TLS_DISABLE_PEER_VERIFICATION=1`。它确实关系到本地连接远端 PolarDB 的可达性，但目前没有完成开启 peer verification 的 A/B 实连证据。因此，不能在 driver 重基线时顺手删除，也不能把它扩散到新的生产启动路径。

### 阶段一：重基线只保持现状，不合并安全策略变更

- [x] 已保留当前 `lpcprj` 的 `MARIADB_TLS_DISABLE_PEER_VERIFICATION=1` 行为和既有 Connector/C 构建条件，没有改变本地开发连接策略。
- [x] 当前环境没有配置 `MUD_ACCOUNT_DB_*`；默认回退目标是未获选择的远端地址，因此本轮没有发起实连，也不声称已完成数据库 A/B。
- [x] 该变量只保留在既有 Windows `lpcprj` 启动边界，没有添加到 Linux、服务端或通用 driver 默认环境。
- [x] 已记录该变量只要存在就会关闭 peer verification，设置为 `0` 也不是安全恢复。
- [x] driver 重基线没有同时发布 TLS 策略改变；数据库证书治理仍可单独定位、验证和回退。

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
- [x] 抽离结果固定在 `a3cbf2bf5815d4fa269712fb4e9c37a2773d4b56`；迁移完成并切换本地 `master` 后，以归档标签 `legacy/extension-extraction-2026-09-15` 保留该可重放、可审查切片，再删除临时分支。该标签未推送，相关代码未部署。

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
- [x] Windows `build/dist`、安装镜像、安装器、`lpcprj` 与 `lpccp` 已接入；`build.cmd` 从干净 `build/work` 完成最终构建，布局、相对配置、安装器语言/配置和用户 PATH 测试通过。
- [x] compile service 已按扩展目录接入；CMake 默认关闭，Windows `build.cmd` 和 Windows CI 显式启用。26 项服务测试通过，并修复 named-pipe 就绪竞态、相对配置路径 ID 漂移和过宽 DACL。
- [x] 江湖英杰传隔离冷启动与测试角色登录已通过。官方新版暴露的两处 mudlib 兼容问题已在 LPC 仓库以最小提交修复：`START_ROOM` 显式引入，以及背包分类服务在自由任务前预加载；它们均不是需要移植的 driver 独有功能。

### Task 6：本地候选环境验证

- [x] 使用江湖英杰传本地工作树的隔离副本和 `config/config.dev`；所有角色数据写入只发生在副本中的 `gameteststd1` 存档。
- [x] 旧、新 driver 使用不同的临时端口、日志目录和配置绝对路径；没有停止或替换原有本地实例。
- [x] 验证只执行冷启动、编译服务请求、测试角色登录和 `look`，未执行支付、QQ 消息、邮件、部署控制或公告操作。
- [x] 候选安装镜像通过 `lpcprj` 完整冷启动 master、simul_efun 和全部 preload，并进入 `Initializations complete`；没有对生产目录批量 reload。
- [x] 已用旧、新 driver 对同一复制角色完成登录和 `look`，并用同一份 32 项 native JSON 契约核对精确序列化/反序列化行为；两处实际语义差异均收敛为 mudlib 最小修复，没有增加 driver 兼容补丁。
- [x] 候选 driver 已从复制的旧测试角色存档恢复并正常登录，候选保存后旧 driver 又成功恢复同一副本并完成登录和 `look`；原 LPC 工作树中的测试存档未被候选实例写入。
- [x] 已进行约 90 秒的本地有界运行观察，覆盖冷启动、heartbeat/call_out 基本运行、断线后再次登录和重复 `look`；常驻内存由 80,896 KiB 降至 80,308 KiB，未出现新增 driver/runtime/save 错误。异步 DB、对象 swap、TLS/WebSocket 握手和长时趋势留给将来的发布前验证，不作为本轮纯本地重基线门槛。
- [x] 最终候选工件已从 clean-source 提交 `a3860bcc` 重建：`driver.exe` SHA-256 为 `c438b3d5d964bc1c561073753ff159dfa3e62fd11bffa47ea334a4ad21a9104d`，`lpccp.exe` 为 `37a2c52ba81b0ccb3d593c05651777e064832c26341dee0ea801a4503d5a0af6`，`lpcprj.exe` 为 `8475446e4deadbf0225ef45c94786fe53ebc44537d6e6c86e2f8dd684334ab4e`，installer 为 `5f8dc5fc8917ccce6efaf0b73aace1795747483f42ab785253de0ee807e76cde`。

### Task 7：本地收尾

- [x] 旧基线与候选基线的差异、验证结果和未验证项已汇总在本计划、独有能力清单和 upstream tracker。
- [x] 已更新 `docs/superpowers/upstream-merge-tracker.md`，记录官方快照、保留能力、删除补丁和本地验证结果。
- [x] 候选通过本地验收后，本地 `master` 以分支指针切换方式接纳新基线；没有把旧 `master` 普通 merge 到候选历史。未推送、未发布、未部署。
- [x] 迁移前 master 的 legacy tag、扩展抽离归档标签与旧 `build/dist` 副本均保留。将来若决定上线，另写简短上线清单并重新取得授权。

---

## 五、验证矩阵

### 构建

- [x] Windows：`build.cmd` 完成，所有支持工件位于 `build/dist`，安装镜像和 installer 同步生成。
- [ ] Linux Release、GitHub `linux-static` 和 sanitizer 留给候选准备推送或发布时执行；本轮明确禁止推送，因此它们不是本地分支迁移完成门槛。workflow 定义及其静态链接/测试门禁已保留。
- [x] `git diff --check` 无错误。
- [x] Windows 候选代码在 `89c107ec` 完成完整验证；最终 clean-source 构建来自 `a3860bcc`，版本为 `20260820-dd2a3a14-a3860bcc`，制品和 SHA-256 已补录，不以文件时间或文件名代替校验。

### Driver 与 LPC

- [x] C++ 非 testsuite CTest 共 366 项通过，0 失败。
- [x] 最终代码对应的 `build/dist/driver.exe` 全量运行官方 LPC testsuite：715 个文件、10,690 项检查通过，输出 `Checks succeeded.`。
- [x] 当前游戏 mudlib 在隔离冷启动中完成 master、simul_efun、全部 preload 和登录路径对象编译；测试角色登录与 `look` 成功。
- [x] master、simul_efun、继承链、clone 和 hot reload 由完整官方 testsuite 覆盖；江湖 owner 另以冷启动、登录和 `lpccp --reload-loaded` 验证。
- [x] 本轮实际发现的新旧语义差异已解释并处理：`START_ROOM` 的隐式头文件依赖和编译期对象加载对应的 preload 顺序均改在 mudlib，不修改官方 core。
- [x] 官方 testsuite 的 save/restore 用例通过；江湖测试角色副本完成旧→新读取/保存以及新→旧再次读取的双向验证。

### 本地独有能力

- [x] 同一份 native JSON 32 项契约在旧、新 driver 上分别通过，包含精确编码字符串、Unicode、循环引用、非法输入和格式化；江湖真实登录协议另由两边的同角色 `look` smoke 覆盖。
- [x] HTTP 请求/响应 parser 的增量、边界和错误用例通过，共 90 项检查。
- [x] mapping instrumentation 经消费者审计后明确不迁移；候选使用官方 mapping 实现，因此不存在本地 mapping 统计钩子的验收项。
- [x] `lpccp` compile、reload-loaded、compile-only、fresh-required 和协议兼容由扩展测试覆盖；最终 dist 对江湖 `/adm/daemons/zone_statusd.c` 返回 `ok: true`、无 diagnostics/runtime_errors。drive 以相对配置启动而客户端使用绝对路径的场景也已通过；目录 reload 不作为本轮验收操作。
- [x] 并发 transport 测试连续重复通过；启动函数现在等待首个 named-pipe 实例就绪，FIFO 串行执行、关闭和超时路径由服务测试覆盖。

### 江湖英杰传本地集成

- [x] Windows 安装器、安装镜像布局和相对配置启动验证通过；runtime ZIP 由保留的 GitHub workflow 在将来获准推送后生成。
- [ ] TLS/WebSocket 实际握手和正常证书验证数据库连接留给将来的发布前验证；本轮不连接未明确选择的远端目标。
- [x] 已使用本地测试角色副本完成登录、断线后再次登录、旧→新→旧存档恢复和 `look`。
- [x] 本地有界运行观察期间无新增 driver 崩溃、runtime error、存档错误或持续内存增长；这不是生产长时 soak 的替代证明。
- [x] 整个验证过程没有触发远程部署、线上热编译或线上玩家数据变更。

---

## 六、回退方案

- [x] legacy tag 指向迁移前基线 `98cc9b42b6f7f1630189b4b968ed708ff41f2203`，没有移动。
- [x] 旧 dist 保存在 `build/legacy-dist-pre-official-rebase-2026-09-15`，新候选位于 `build/dist`，两者相互独立。
- [x] 候选实例已停止；回退只需使用 legacy tag/旧 dist，不修改线上环境。
- [x] 本地存档测试只操作隔离副本，原测试角色存档未被候选实例覆盖。

---

## 七、完成标准

迁移只有同时满足以下条件才算完成：

- 官方稳定 tag 来源和 commit 已固定并可复核。
- 当前 FluffOS 和江湖英杰传本地 Git 基线已记录并可复核。
- 本地独有能力均有明确归属：保留、重写、由官方替代或删除。
- 江湖英杰传的静态、动态和外部消费者依赖已形成机器可比较契约。
- 官方核心文件中的本地改动收敛到少量、有测试的扩展钩子。
- `build/dist`、安装器、`lpccp`、`lpcprj`、JSON 和 HTTP 全部通过验证；无消费者的 mapping 统计明确不迁移。
- `MARIADB_TLS_DISABLE_PEER_VERIFICATION=1` 按现有本地启动兼容条件只保留在 `lpcprj`；移除它及数据库证书治理是独立发布前任务，不属于本次重基线完成条件。
- 完整 testsuite、实际 mudlib 和本地测试角色 smoke test 均有证据。
- 候选可以读取并保存本地测试角色存档，原存档副本未被覆盖。
- 候选制品经过本地验证并记录 SHA-256。
- legacy 版本可恢复，迁移期间没有删除唯一回退引用。
- 没有对线上服务器、线上进程或线上玩家数据执行任何操作。

## 八、开始代码迁移前的条件

开始建立官方基线分支前只要求三件事：

- [x] 当前 FluffOS `master` 已建立 legacy tag，可以从 Git 恢复。
- [x] 已列出江湖英杰传真正依赖的本地独有能力，包括 JSON、HTTP parser、Windows 交付、轻量 CI 和 compile service。
- [x] 当前 `master` 保留 legacy tag，新迁移在独立分支进行，不影响现有生产维护。

本地存档验证和候选运行属于迁移后期的本地验收。生产切换不在本计划范围内；将来如需上线，必须作为新的独立任务重新确认。
