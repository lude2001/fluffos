# FluffOS 官方版本合并跟踪表

本文档只记录当前维护版与官方上游之间的重要边界。迁移前旧分支逐批回移官方 PR 的明细属于历史记录，不再作为当前代码的“未合入清单”。

## 当前基线

- 官方上游：[`fluffos/fluffos`](https://github.com/fluffos/fluffos)
- 本维护版：[`lude2001/fluffos`](https://github.com/lude2001/fluffos)
- 官方稳定版本：`v2026.0901.0`
- 官方基线提交：`7af5c3fffe2505c7cb764951bed863b16eee471b`
- 官方提交时间：`2026-08-31T21:17:12-07:00`
- 本次审查及重建日期：`2026-09-15`
- 迁移前版本：`98cc9b42b6f7f1630189b4b968ed708ff41f2203`
- 迁移前归档标签：`legacy/pre-official-rebase-2026-09-15`
- 独有功能抽离归档标签：`legacy/extension-extraction-2026-09-15`，指向 `a3cbf2bf5815d4fa269712fb4e9c37a2773d4b56`

本维护版直接以官方稳定提交为源码基线，再叠加少量本地能力；没有把迁移前 `master` 的 130 个本地提交整体 merge、rebase 或机械 cherry-pick 到新基线上。

## 本地保留能力

### 原生 JSON package

- 路径：`src/packages/json/`
- efun：`json_encode()`、`json_decode()`、`json_format()`
- 原因：江湖 mudlib 的登录、客户端协议、autoload、配置和业务数据均依赖既有 native JSON 语义。
- 边界：保持独立 package，不改官方 JSON 标准库实现。

### HTTP helper 与流式解析 package

- 路径：`src/packages/lude_http/`
- 能力：URL/query/form 辅助函数、HTTP 响应构造、请求和响应增量解析。
- 原因：江湖 mudlib 的 HTTP 服务和外部集成入口实际使用这些 efun。
- 边界：独立于官方 sockets package。

### 运行时编译服务与 `lpccp`

- 路径：`src/extensions/compile_service/`
- 用途：向运行中的 VM 发起编译、检查和受控重载请求。
- 边界：CMake 默认关闭，仅本仓库 Windows 构建和 Windows CI 显式设置 `ENABLE_LUDE_COMPILE_SERVICE=ON`；核心 driver 只保留少量条件编译钩子。
- 安全：Windows named pipe 只允许当前用户、SYSTEM 和 Administrators。

### Windows 交付工具

- `build.cmd` 是 Windows 规范构建入口。
- `build/dist` 是唯一支持的本地 Windows 运行工件目录。
- 保留 `lpcprj`、安装器、DLL 自动分阶段打包和相对配置启动支持。
- `MARIADB_TLS_DISABLE_PEER_VERIFICATION=1` 仅按既有行为保留在本地 `lpcprj` 启动边界；移除它和建立正常数据库证书验证属于独立任务。

### 轻量 CI/CD

本仓库只保留以下工作流：

- `.github/workflows/ci-ubuntu.yml`
- `.github/workflows/ci-macos.yml`
- `.github/workflows/ci-windows.yml`
- `.github/workflows/release-artifacts.yml`

这些工作流继续提供跨平台测试和 Linux 静态生产 driver 工件，不采用官方仓库更重的 CI、自动发布、Docker、CodeQL、Coverity 和文档站工作流。

## 明确替换或不迁移的内容

- **官方功能不排除：** `v2026.0901.0` 已包含的编译器、VM、async、FFI、WASM、网络、第三方依赖和修复均属于当前基线，不能再列为“尚未合入”。
- **旧回移实现不重放：** `recompile_object()`、`sys_reload_tls()`、`get_os_env()`、`set_os_env()`、`request_clean_up()`、`set_clean_up()`、`to_buffer()` 等使用官方稳定版实现，只保留必要兼容测试。
- **mapping 插桩不迁移：** 江湖 mudlib 和本地工具没有发现 mapping 创建程序归因统计的非测试消费者，因此候选版本使用官方 mapping 实现。
- **官方工作流被替换：** 官方源码功能仍保留，但官方重型 CI/release workflow 被本仓库四个轻量工作流替换。
- **旧 third-party 和自动生成文件不迁移：** 采用官方稳定基线中的版本和布局，不搬运迁移前分支的 vendor 快照或旧编译器生成文件。

## 关键验证结果

### 官方原始基线

- Windows/MSYS2 MinGW64 RelWithDebInfo 配置、构建和安装成功。
- 340 个非 testsuite CTest 用例全部通过。
- 隔离端口 testsuite 通过 711 个 LPC 文件、10,655 项检查，并输出 `Checks succeeded.`。

### 本地能力叠加后

- Windows `build.cmd` 完成，`build/dist`、安装映像和双语安装器生成成功。
- 366 个非 testsuite CTest 用例全部通过。
- 完整 LPC testsuite 通过 715 个文件、10,690 项检查。
- native JSON 的 32 项本地契约在迁移前和新 driver 上均通过；官方 LPC JSON 套件的 178 项检查继续通过。
- HTTP helper、request parser 和 response parser 共 90 项检查通过。
- compile service 共 26 项测试通过；真实 `lpccp` 请求返回 `ok: true`，diagnostics 和 runtime errors 均为空。
- 江湖 mudlib 隔离冷启动完成全部 preload；复制的 `gameteststd1` 存档能够登录并执行 `look`。
- 测试存档完成“旧 driver 读取 → 新 driver 恢复并保存 → 旧 driver 再次恢复”的双向验证。
- 断线重连及 90 秒本地有界观察期间，没有新增编译、运行时或存档错误。

最终干净源码工件从 `a3860bcc` 构建，版本为 `20260820-dd2a3a14-a3860bcc`：

| 工件 | SHA-256 |
| --- | --- |
| `driver.exe` | `c438b3d5d964bc1c561073753ff159dfa3e62fd11bffa47ea334a4ad21a9104d` |
| `lpccp.exe` | `37a2c52ba81b0ccb3d593c05651777e064832c26341dee0ea801a4503d5a0af6` |
| `lpcprj.exe` | `8475446e4deadbf0225ef45c94786fe53ebc44537d6e6c86e2f8dd684334ab4e` |
| Windows installer | `5f8dc5fc8917ccce6efaf0b73aace1795747483f42ab785253de0ee807e76cde` |

## 尚未执行的发布前验证

以下项目不是“官方功能尚未合入”，而是当前纯本地迁移没有执行的验证：

- Linux Release、GitHub 静态构建和 sanitizer 实际运行；
- TLS/WebSocket 真实握手；
- 启用 peer/hostname verification 的数据库 TLS 连接；
- 更广泛的江湖玩法对比；
- 生产时长级别的稳定性观察。

这些项目必须在将来的推送或发布任务中单独完成。本次迁移没有连接、修改或重启线上服务器，也没有操作线上玩家数据。

## 历史记录边界

迁移前旧 `master` 曾逐批选择性回移 PR #1210 至 #1261 附近的官方功能和修复。那些“已合入/尚未合入”结论只描述旧提交 `00cf1f21` 所在阶段；当前版本已经直接采用更晚的官方 `v2026.0901.0`，因此不再逐条维护这些旧结论。

需要审计旧实现时，使用：

- `legacy/pre-official-rebase-2026-09-15`：迁移前完整源码；
- `legacy/extension-extraction-2026-09-15`：旧基线上的独有功能模块化切片；
- [独有能力清单](plans/2026-09-15-official-stable-unique-capability-inventory.md)；
- [迁移计划](plans/2026-09-15-official-stable-rebase-migration.md)。

## 后续更新规则

采用新的官方稳定版本时：

1. 只读查询 `fluffos/fluffos`，记录正式 tag、解引用提交和日期。
2. 先验证未修改的官方基线，再叠加本地能力。
3. 只记录仍存在的本地 package、工具、工作流和少量核心钩子，不恢复逐 PR 流水账。
4. 区分“源码没有采用”“被本地方案替换”和“尚未执行验证”，不能混写为“尚未合入”。
5. 记录完整测试结果、跳过项和发布边界。
6. 官方仓库始终只读；不得为其添加 remote、推送分支、编辑 PR 或执行 release。
