# 跨平台编译验证设计

**日期**: 2026-06-24
**状态**: 已实施

## 背景与问题

FluffOS 源码中 `src/compile_service.cc` 无条件引入 `windows.h` / `sddl.h`，但其内部的命名管道传输逻辑（`CreateNamedPipeA`、`ConnectNamedPipe`、SDDL 安全描述符等）是 Windows 专属。该文件被 `src/CMakeLists.txt` 无条件编入 `libdriver`，且 `backend.cc`、`mainlib.cc` 无条件调用其公开接口，导致 Linux/macOS 上完全无法编译。

此外，`.github/workflows/` 目录为空——没有任何 CI 在保证跨平台可编译性。唯一的验证手段是本机 `build.cmd`（仅 Windows）。

## 安全约束

- 所有 Git 写操作仅针对 `origin`（`lude2001/fluffos`），仅在本地分支开发、本地合并、再推 `master`。
- 严禁对官方 fluffos 仓库（`fluffos/fluffos`）做任何写操作。
- 重建 CI 时可**只读参考**官方项目的 workflow 配置。

## 设计决策

### 决策 1：compile_service 跨平台处理 — 文件内 `#ifdef _WIN32` 精确隔离

**原方案**：拆分为 `compile_service_win.cc` + `compile_service_stub.cc`（空实现）。

**实施中的发现与调整**：`compile_service.cc` 的代码实际分两层：
- **队列/分发层**（PendingRequest、dispatch_compile_service_request、process_compile_service_requests、fail_queued_requests 等）：纯标准 C++（mutex/condition_variable/json），跨平台。
- **命名管道传输层**（PipeSecurityAttributes、handle_client、service_loop、start/stop 中的管道操作）：Windows 专属。

`test_compile_service.cc` 的多个 lifecycle 测试强依赖队列层（如 `ASSERT_TRUE(start_compile_service(...))`、FIFO 顺序验证）。纯空 stub 会导致 `make test` 失败。

**最终方案**：保持单文件，用 `#ifdef _WIN32` 精确隔离 5 处 Windows 专属代码：
1. `#include <windows.h>` / `<sddl.h>`
2. `kPipeSecurityDescriptor` 常量 + `PipeSecurityAttributes` 类
3. `handle_client()` + `service_loop()` 函数
4. `start_compile_service()` 中的 `g_server_thread = std::thread(service_loop)`
5. `stop_compile_service()` 中的 `CreateFileA` 管道打断

非 Windows 路径：`start_compile_service` 仍置 `g_running=true`（队列可用），但不启管道线程；`stop_compile_service` 跳过管道打断，其余清理照常。所有 lifecycle 测试在非 Windows 上通过（队列逻辑跨平台），唯一的管道传输测试（test_compile_service.cc:720-783）已有 `#ifdef _WIN32` 守卫。

**未改动**：`compile_service_protocol.h`、`compile_service_client.h`、`main_lpccp.cc`（均已正确守卫）；`runtime_compile_request.cc`、`runtime_dev_test_request.cc`（本就跨平台）。

### 决策 2：验证机制 — GitHub Actions 三平台独立 workflow

三个独立 workflow 文件（便于定位失败），均遵循官方 fluffos 的验证过的配置：

| 文件 | 平台 | 矩阵 | 总 job 数 |
|------|------|------|-----------|
| `ci-ubuntu.yml` | ubuntu-24.04 | gcc×clang × Debug×RelWithDebInfo | 4 |
| `ci-macos.yml` | macos-14 | Debug×RelWithDebInfo | 2 |
| `ci-windows.yml` | windows-latest (MSYS2) | Debug×RelWithDebInfo | 2 |

每个 job 的步骤：checkout → 装依赖 → cmake configure → make install → make test → LPC testsuite。

### 决策 3：CI 范围 — 仅验证可编译

CI 只验证三平台 configure + build + make test + LPC 测试套件通过，不产出发布工件。发布仍由 Windows 的 `build.cmd` + installer 负责。

## 实施清单

### 源码改动

- `src/compile_service.cc`：5 处 `#ifdef _WIN32` 隔离（如上所述）。

### CI 改动

- `.github/workflows/ci-ubuntu.yml`：新建，4 个 job（gcc/clang × Debug/RelWithDebInfo）。
- `.github/workflows/ci-macos.yml`：新建，2 个 job（Debug/RelWithDebInfo），含 `OPENSSL_ROOT_DIR` / `ICU_ROOT`。
- `.github/workflows/ci-windows.yml`：新建，2 个 job（Debug/RelWithDebInfo），MSYS2/MINGW64。

## 验证

- Windows 本地增量构建 `libdriver` + `lpc_tests` 目标：通过，无错误。
- 三个 workflow YAML 语法验证：通过。
- Linux/macOS 编译验证：需推送到 GitHub 触发 CI（本机无法验证）。

## 未来扩展

- 若需在非 Windows 上启用远程编译服务（lpccp 客户端），可实现 Unix domain socket (AF_UNIX) 传输层，复用现有队列/分发逻辑。
- 若需 sanitizer/CodeQL/coverity，可增量添加对应 workflow。
- 若需多平台发布工件，可在现有 job 中追加 upload 步骤。
