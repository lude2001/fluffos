# FluffOS — lude2001 维护版

[English](README.md)

本仓库是我们独立维护的 FluffOS 发行版本，主要服务于自己的 LPC 游戏项目，包括《江湖英杰传》。源码公开，兼容的 LPC mudlib 和相关工具可以复用；但本仓库的首要目标始终是保障我们自身生产技术栈的稳定性和实际需求，而不是承担通用发行版的完整支持职责。

本仓库**不是 FluffOS 官方仓库**。

## 仓库关系

| 角色 | 地址 |
| --- | --- |
| 官方上游 | [`fluffos/fluffos`](https://github.com/fluffos/fluffos) |
| 本维护版 | [`lude2001/fluffos`](https://github.com/lude2001/fluffos) |
| 官方文档 | [fluffos.info](https://www.fluffos.info/) |

当前代码已经以官方稳定版 `v2026.0901.0` 重新建立基线。我们只读审查官方版本并按需要主动合入，不会持续镜像官方 `master`。

[官方版本合并跟踪表](docs/superpowers/upstream-merge-tracker.md)记录精确的官方快照、本地保留能力、验证证据和有意不合入的内容；[迁移计划](docs/superpowers/plans/2026-09-15-official-stable-rebase-migration.md)记录本次重建基线及回退边界。

## 本维护版保留的能力

driver 仍兼容常规 FluffOS/MudOS 风格的 LPC mudlib。我们维护的独有能力尽量限制在 package、独立工具或少量集成边界内：

- `src/packages/json/` 中的原生 `json_encode()`、`json_decode()` 和 `json_format()` efun；
- `src/packages/lude_http/` 中的 HTTP 辅助函数及流式请求/响应解析器；
- `src/extensions/compile_service/` 中默认关闭的运行时编译服务和 `lpccp` 客户端；
- Windows `build/dist`、`lpcprj`、安装器和分阶段打包脚本；
- 本仓库维护的四个轻量 CI/制品工作流。

官方稳定版已经提供的功能优先采用官方实现。对编译器或 VM 内部的本地修改必须保持少量、有测试，并由真实消费者或可复现的兼容问题证明其必要性。

## 构建

### Windows

Windows 唯一支持的规范入口是：

```powershell
.\build.cmd
```

请在安装 MSYS2/MinGW64 后，从仓库根目录运行。所有受支持的 Windows 运行工件只发布到 `build/dist`，包括 `driver.exe`、`lpccp.exe`、`lpcprj.exe`、必需 DLL、头文件、标准 LPC 文件和启动脚本。

`build.cmd` 会启用本地编译服务扩展。自定义 CMake 构建默认不启用，需要时显式指定：

```text
-DENABLE_LUDE_COMPILE_SERVICE=ON
```

### Linux 和 macOS

使用标准 CMake 流程：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMARCH_NATIVE=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

平台依赖和其他构建目标可参考 [FluffOS 官方仓库](https://github.com/fluffos/fluffos)及本仓库的 [`AGENTS.md`](AGENTS.md)。本仓库实际维护的构建配置以 `.github/workflows/` 为准。

## 测试

运行 CTest：

```bash
ctest --test-dir build --output-on-failure
```

使用已安装的 Unix driver 运行 LPC testsuite：

```bash
cd testsuite
../build/bin/driver etc/config.test -ftest
```

使用规范 Windows 构建：

```powershell
cd testsuite
..\build\dist\driver.exe etc\config.test -ftest
```

修改 efun、编译器、VM 状态、持久化、网络或数据库行为时，必须增加有针对性的回归覆盖；仅构建成功不能作为完成标准。

## 重要目录

- `src/`：driver、编译器、VM、网络和 package 源码。
- `src/packages/`：模块化 efun 包。
- `src/extensions/`：可选的本地集成。
- `testsuite/`：C++/LPC 集成和回归测试。
- `docs/`：driver 与 LPC 文档。
- `.github/workflows/`：本维护版的轻量 CI 和构建制品流程。
- `build/dist/`：受支持的本地 Windows 运行工件目录。

## 合入官方更新

采用新的官方版本时：

1. 以只读方式检查 `fluffos/fluffos`，固定一个官方稳定 tag；
2. 先验证未修改的官方基线；
3. 只重新接入仍有真实消费者的本地 package、工具、工作流和最小 hook；
4. 完成 driver、LPC、实际 mudlib 和存档兼容验证；
5. 在同一变更集中更新[官方版本合并跟踪表](docs/superpowers/upstream-merge-tracker.md)。

不要把本仓库当作自动同步 fork，也不要把本维护版的分支或 release 操作发送到官方仓库。

## 范围与版权

Issue 和改动首先按我们的生产需求评估。在不破坏这一边界的前提下，可以接受对其他 LPC 项目有帮助的兼容改进；但本仓库不替代 FluffOS 官方支持渠道。

源码的使用与再分发遵循 [`Copyright`](Copyright) 中的声明，以及各第三方组件自身的许可证。
