# Windows LPC Runtime Installer Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 FluffOS 增加一个仅面向 Windows 第一阶段的可安装 LPC 运行时发行链路，生成可自定义安装目录、安装完成后可选写入用户级 `PATH` 的 Inno Setup 安装包，并保留现有 zip 分发。

**Architecture:** 继续以 `build/dist` 作为 Windows 可运行 staging 边界，在其后新增稳定的 `build/install-image` 安装镜像层，再由 Inno Setup 从安装镜像生成 installer。运行时入口新增 `lpcprj.exe` 包装器，负责定位内部 `driver.exe`、切换到 `config-file` 所在目录并转发参数；`lpccp.exe` 第一阶段继续直接暴露。发布层同时产出 zip 与 installer，保持本地构建、测试、CI 和发布之间的一致性。

**Tech Stack:** C++17, CMake, PowerShell, Inno Setup, MSYS2/MinGW64, GitHub Actions

---

## 文件结构

- Create: `D:/code/fluffos/src/main_lpcprj.cc`
  - 新的 Windows/跨平台稳定入口包装器，负责调用内部 `driver.exe`。
- Modify: `D:/code/fluffos/src/CMakeLists.txt`
  - 新增 `lpcprj` 目标，并把安装镜像需要的二进制布局与现有目标关联起来。
- Create: `D:/code/fluffos/scripts/stage-windows-install-image.ps1`
  - 将 `build/dist` 重排为稳定安装布局 `build/install-image`。
- Create: `D:/code/fluffos/scripts/build-windows-installer.ps1`
  - 调用 Inno Setup 生成 installer，并接受版本号、镜像目录和输出目录参数。
- Create: `D:/code/fluffos/scripts/test-install-image-layout.ps1`
  - 验证 `build/install-image` 是否包含预期目录和入口文件。
- Create: `D:/code/fluffos/scripts/test-lpcprj-relative-config.ps1`
  - 验证 `lpcprj` 以 `config-file` 所在目录作为工作目录运行。
- Create: `D:/code/fluffos/packaging/windows/fluffos.iss`
  - Inno Setup 安装器脚本。
- Modify: `D:/code/fluffos/build.cmd`
  - 在 `build/dist` 之后串接安装镜像 staging。
- Modify: `D:/code/fluffos/scripts/stage-driver-dist.ps1`
  - 保持现有 dist 边界，同时为后续安装镜像整理暴露稳定输入。
- Modify: `D:/code/fluffos/.github/workflows/release.yml`
  - 保留 zip 上传，并新增 installer 构建与上传。
- Modify: `D:/code/fluffos/docs/superpowers/specs/2026-03-26-windows-lpc-runtime-installer-design.md`
  - 仅在实现时出现偏差时回填设计决策；默认不再扩写。
- Modify: `D:/code/fluffos/README.md`
  - 补充 Windows 安装器与 `lpcprj`/`lpccp` 使用说明。
- Modify: `D:/code/fluffos/README_CN.md`
  - 同步中文说明。

---

## Chunk 1: Install-image staging contract and failing validations

### Task 1: 先补安装镜像布局验证脚本

**Files:**
- Create: `D:/code/fluffos/scripts/test-install-image-layout.ps1`

- [ ] **Step 1: 写出安装镜像契约检查脚本**

脚本至少验证以下路径存在：

- `bin/lpcprj.exe`
- `bin/lpccp.exe`
- `libexec/fluffos/driver.exe`
- `share/fluffos/include`
- `share/fluffos/std`
- `share/fluffos/www`

- [ ] **Step 2: 在当前实现上运行它，确认先失败**

运行：

```powershell
& D:\code\fluffos\scripts\test-install-image-layout.ps1 -InstallImageDir D:\code\fluffos\build\install-image
```

预期：

- 因为 `build/install-image` 尚不存在而失败

- [ ] **Step 3: 提交**

```powershell
git add D:/code/fluffos/scripts/test-install-image-layout.ps1
git commit -m "test: add install image layout validation"
```

### Task 2: 先补 `lpcprj` 相对配置路径的失败测试

**Files:**
- Create: `D:/code/fluffos/scripts/test-lpcprj-relative-config.ps1`
- Reference: `D:/code/fluffos/scripts/test-run-driver-relative-config.ps1`

- [ ] **Step 1: 参考现有 `run-driver` 测试脚本写出 `lpcprj` 验证**

脚本需要：

- 接收 `lpcprj.exe` 路径
- 在临时目录中写入 `config.hell`
- 从非配置目录启动 `lpcprj config.hell`
- 捕获退出码和输出

- [ ] **Step 2: 在当前实现上运行，确认先失败**

运行：

```powershell
& D:\code\fluffos\scripts\test-lpcprj-relative-config.ps1 -LpcprjPath D:\code\fluffos\build\install-image\bin\lpcprj.exe
```

预期：

- 因为 `lpcprj.exe` 尚不存在而失败

- [ ] **Step 3: 提交**

```powershell
git add D:/code/fluffos/scripts/test-lpcprj-relative-config.ps1
git commit -m "test: add lpcprj relative config smoke test"
```

### Task 3: 新增安装镜像 staging 脚本并接到 `build.cmd`

**Files:**
- Create: `D:/code/fluffos/scripts/stage-windows-install-image.ps1`
- Modify: `D:/code/fluffos/build.cmd`
- Modify: `D:/code/fluffos/scripts/stage-driver-dist.ps1`

- [ ] **Step 1: 先明确 `build/dist` 仍是 staging 输入，不直接改成安装布局**

保留：

- `build/dist/driver.exe`
- `build/dist/lpccp.exe`
- `build/dist/include`
- `build/dist/std`
- `build/dist/www`

- [ ] **Step 2: 实现 `stage-windows-install-image.ps1`**

脚本职责：

- 读取 `build/dist`
- 生成 `build/install-image`
- 建立安装布局：
  - `bin/`
  - `libexec/fluffos/`
  - `share/fluffos/`
  - `runtime/` 或与最终布局一致的 DLL 位置

- [ ] **Step 3: 更新 `build.cmd`，让其在 dist staging 后继续生成 install-image**

运行：

```powershell
& D:\code\fluffos\build.cmd
```

预期：

- `build/dist` 继续生成
- `build/install-image` 新增生成

- [ ] **Step 4: 运行布局验证脚本，确认转绿**

运行：

```powershell
& D:\code\fluffos\scripts\test-install-image-layout.ps1 -InstallImageDir D:\code\fluffos\build\install-image
```

预期：

- 路径检查通过

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/scripts/stage-windows-install-image.ps1 D:/code/fluffos/build.cmd D:/code/fluffos/scripts/stage-driver-dist.ps1 D:/code/fluffos/scripts/test-install-image-layout.ps1
git commit -m "build: stage windows install image from dist"
```

---

## Chunk 2: `lpcprj.exe` wrapper and runtime behavior

### Task 4: 先把 `lpcprj` 目标接进构建系统

**Files:**
- Create: `D:/code/fluffos/src/main_lpcprj.cc`
- Modify: `D:/code/fluffos/src/CMakeLists.txt`

- [ ] **Step 1: 在 `src/CMakeLists.txt` 中新增 `lpcprj` 可执行目标**

实现方式参照：

- `driver`
- `lpcc`
- `lpccp`

但不要替换现有 `driver` 目标；`driver` 仍作为内部实现保留。

- [ ] **Step 2: 在 `main_lpcprj.cc` 里先写最小入口并确认可编译**

最小版本只需：

- 解析参数
- 找到安装根目录
- 构造内部 `driver.exe` 路径

- [ ] **Step 3: 构建目标**

运行：

```powershell
cmake -S D:\code\fluffos -B D:\code\fluffos\build\work
cmake --build D:\code\fluffos\build\work --target lpcprj driver lpccp
```

预期：

- `lpcprj.exe` 生成成功

- [ ] **Step 4: 提交**

```powershell
git add D:/code/fluffos/src/main_lpcprj.cc D:/code/fluffos/src/CMakeLists.txt
git commit -m "feat: add lpcprj launcher target"
```

### Task 5: 实现 `lpcprj` 的工作目录与参数转发行为

**Files:**
- Modify: `D:/code/fluffos/src/main_lpcprj.cc`
- Modify: `D:/code/fluffos/scripts/stage-windows-install-image.ps1`
- Modify: `D:/code/fluffos/scripts/test-lpcprj-relative-config.ps1`

- [ ] **Step 1: 先运行 `lpcprj` 冒烟测试，确认它先失败**

运行：

```powershell
& D:\code\fluffos\scripts\test-lpcprj-relative-config.ps1 -LpcprjPath D:\code\fluffos\build\install-image\bin\lpcprj.exe
```

预期：

- 失败原因是包装器尚未正确切换工作目录、或未能定位 `driver.exe`

- [ ] **Step 2: 在 `main_lpcprj.cc` 中实现核心启动逻辑**

要求：

- 参数 1 视为 `config-file`
- 将工作目录切换到 `config-file` 所在目录
- 调用 `libexec/fluffos/driver.exe`
- 原样转发剩余参数
- 返回内部 `driver.exe` 的退出码

- [ ] **Step 3: 处理 DLL 搜索路径**

至少保证以下任一策略成立：

- 运行前把 `runtime/` 加入 DLL 搜索路径
- 或将所需 DLL 布置为与实际执行二进制相邻

- [ ] **Step 4: 重新生成安装镜像并运行测试**

运行：

```powershell
& D:\code\fluffos\build.cmd
& D:\code\fluffos\scripts\test-lpcprj-relative-config.ps1 -LpcprjPath D:\code\fluffos\build\install-image\bin\lpcprj.exe
```

预期：

- `lpcprj` 可以从非配置目录启动
- 使用 `config-file` 所在目录作为运行目录

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/src/main_lpcprj.cc D:/code/fluffos/scripts/stage-windows-install-image.ps1 D:/code/fluffos/scripts/test-lpcprj-relative-config.ps1
git commit -m "feat: make lpcprj launch driver from config directory"
```

### Task 6: 把 `lpccp.exe` 放入稳定安装布局并验证可见性

**Files:**
- Modify: `D:/code/fluffos/scripts/stage-windows-install-image.ps1`
- Modify: `D:/code/fluffos/scripts/test-install-image-layout.ps1`

- [ ] **Step 1: 明确第一阶段 `lpccp.exe` 直接暴露**

将 `build/dist/lpccp.exe` 放入：

- `build/install-image/bin/lpccp.exe`

- [ ] **Step 2: 若 `lpccp` 依赖 DLL，确保安装镜像中可运行**

复用与 `lpcprj` 一致的 DLL 收纳策略。

- [ ] **Step 3: 重新跑布局验证**

运行：

```powershell
& D:\code\fluffos\scripts\test-install-image-layout.ps1 -InstallImageDir D:\code\fluffos\build\install-image
```

预期：

- `lpccp.exe` 检查通过

- [ ] **Step 4: 提交**

```powershell
git add D:/code/fluffos/scripts/stage-windows-install-image.ps1 D:/code/fluffos/scripts/test-install-image-layout.ps1
git commit -m "build: publish lpccp in windows install image"
```

---

## Chunk 3: Inno Setup installer and PATH integration

### Task 7: 新增 Inno Setup 脚本骨架并先生成失败/占位 installer

**Files:**
- Create: `D:/code/fluffos/packaging/windows/fluffos.iss`
- Create: `D:/code/fluffos/scripts/build-windows-installer.ps1`

- [ ] **Step 1: 新建 `fluffos.iss` 基础脚本**

至少定义：

- `AppName`
- `AppVersion`
- `DefaultDirName`
- 安装镜像文件来源
- 输出 installer 文件名

- [ ] **Step 2: 新建 `build-windows-installer.ps1`**

脚本参数至少包含：

- `InstallImageDir`
- `OutputDir`
- `Version`
- `InnoSetupCompilerPath`（可选）

- [ ] **Step 3: 在无 `ISCC.exe` 的情况下给出清晰失败信息**

运行：

```powershell
& D:\code\fluffos\scripts\build-windows-installer.ps1 -InstallImageDir D:\code\fluffos\build\install-image -OutputDir D:\code\fluffos\build -Version 0.0.0-test
```

预期：

- 如果本机未安装 Inno Setup，失败信息明确指出缺少 `ISCC.exe`

- [ ] **Step 4: 提交**

```powershell
git add D:/code/fluffos/packaging/windows/fluffos.iss D:/code/fluffos/scripts/build-windows-installer.ps1
git commit -m "build: add inno setup packaging scaffolding"
```

### Task 8: 实现自定义安装目录和安装后 PATH 询问

**Files:**
- Modify: `D:/code/fluffos/packaging/windows/fluffos.iss`

- [ ] **Step 1: 在安装器中启用默认目录但允许用户修改**

默认目录：

```text
{autopf}\FluffOS
```

但不得锁死目录选择。

- [ ] **Step 2: 增加安装完成页的“加入 PATH”选项**

要求：

- 选项出现在安装完成后
- 用户可选择是否加入 `PATH`
- 目标为用户级 `PATH`

- [ ] **Step 3: 将写入逻辑限定为 `bin\`**

不要把根目录写入 PATH，只写：

```text
<install-root>\bin
```

- [ ] **Step 4: 若本机已安装 Inno Setup，编译 installer 验证脚本语法**

运行：

```powershell
& D:\code\fluffos\scripts\build-windows-installer.ps1 -InstallImageDir D:\code\fluffos\build\install-image -OutputDir D:\code\fluffos\build -Version 0.0.0-test
```

预期：

- 生成测试 installer

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/packaging/windows/fluffos.iss
git commit -m "feat: add user path option to windows installer"
```

### Task 9: 验证安装器产物命名与 zip 并存

**Files:**
- Modify: `D:/code/fluffos/scripts/build-windows-installer.ps1`

- [ ] **Step 1: 让脚本输出稳定命名**

输出目标：

```text
fluffos-<version>-windows-x86_64-installer.exe
```

- [ ] **Step 2: 不覆盖 zip 产物命名**

保留：

```text
fluffos-<version>-windows-x86_64.zip
```

- [ ] **Step 3: 再次生成 installer**

运行与上一任务相同。

- [ ] **Step 4: 提交**

```powershell
git add D:/code/fluffos/scripts/build-windows-installer.ps1
git commit -m "build: standardize windows installer artifact name"
```

---

## Chunk 4: Release workflow and documentation

### Task 10: 更新 GitHub Release 流程，同时上传 zip 和 installer

**Files:**
- Modify: `D:/code/fluffos/.github/workflows/release.yml`

- [ ] **Step 1: 在 Windows release job 中安装 Inno Setup**

可采用适合 GitHub Actions Windows runner 的安装方式，但不要移除现有 zip 构建逻辑。

- [ ] **Step 2: 让 workflow 在 zip 之外再构建 installer**

要求：

- 继续上传 zip
- 新增 installer 上传
- release notes 反映两种工件

- [ ] **Step 3: 检查 release notes 文案**

将 Windows 安装说明区分为：

- zip 手动解压运行
- installer 安装后使用 `lpcprj` / `lpccp`

- [ ] **Step 4: 提交**

```powershell
git add D:/code/fluffos/.github/workflows/release.yml
git commit -m "ci: publish windows installer alongside zip"
```

### Task 11: 更新用户文档

**Files:**
- Modify: `D:/code/fluffos/README.md`
- Modify: `D:/code/fluffos/README_CN.md`

- [ ] **Step 1: 在英文 README 中补充 Windows 安装器用法**

至少说明：

- installer 与 zip 两种分发方式
- `lpcprj <config-file>`
- `lpccp`
- 安装完成后可选加入 PATH

- [ ] **Step 2: 在中文 README 中同步说明**

保持与英文文档的行为描述一致。

- [ ] **Step 3: 提交**

```powershell
git add D:/code/fluffos/README.md D:/code/fluffos/README_CN.md
git commit -m "docs: document windows runtime installer usage"
```

---

## Chunk 5: Final verification

### Task 12: 做完整本地回归验证

**Files:**
- Verify: `D:/code/fluffos/build.cmd`
- Verify: `D:/code/fluffos/scripts/stage-windows-install-image.ps1`
- Verify: `D:/code/fluffos/scripts/build-windows-installer.ps1`
- Verify: `D:/code/fluffos/packaging/windows/fluffos.iss`
- Verify: `D:/code/fluffos/src/main_lpcprj.cc`

- [ ] **Step 1: 全量生成 dist 和 install-image**

运行：

```powershell
& D:\code\fluffos\build.cmd
```

预期：

- `build/dist` 存在
- `build/install-image` 存在

- [ ] **Step 2: 跑安装镜像布局验证**

运行：

```powershell
& D:\code\fluffos\scripts\test-install-image-layout.ps1 -InstallImageDir D:\code\fluffos\build\install-image
```

预期：

- 通过

- [ ] **Step 3: 跑 `lpcprj` 相对配置路径测试**

运行：

```powershell
& D:\code\fluffos\scripts\test-lpcprj-relative-config.ps1 -LpcprjPath D:\code\fluffos\build\install-image\bin\lpcprj.exe
```

预期：

- 通过

- [ ] **Step 4: 若 Inno Setup 已安装，生成最终测试 installer**

运行：

```powershell
& D:\code\fluffos\scripts\build-windows-installer.ps1 -InstallImageDir D:\code\fluffos\build\install-image -OutputDir D:\code\fluffos\build -Version 0.0.0-test
```

预期：

- 产出 `fluffos-0.0.0-test-windows-x86_64-installer.exe`

- [ ] **Step 5: 检查工作树并提交收尾**

运行：

```powershell
git status --short
```

确认只包含预期文件后提交：

```powershell
git add D:/code/fluffos/src/main_lpcprj.cc D:/code/fluffos/src/CMakeLists.txt D:/code/fluffos/scripts/stage-windows-install-image.ps1 D:/code/fluffos/scripts/build-windows-installer.ps1 D:/code/fluffos/scripts/test-install-image-layout.ps1 D:/code/fluffos/scripts/test-lpcprj-relative-config.ps1 D:/code/fluffos/packaging/windows/fluffos.iss D:/code/fluffos/build.cmd D:/code/fluffos/scripts/stage-driver-dist.ps1 D:/code/fluffos/.github/workflows/release.yml D:/code/fluffos/README.md D:/code/fluffos/README_CN.md
git commit -m "feat: add windows lpc runtime installer"
```

Plan complete and saved to `docs/superpowers/plans/2026-03-26-windows-lpc-runtime-installer.md`. Ready to execute?
