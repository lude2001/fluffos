# AGENTS.md

此文件为 Codex (Codex.ai/code) 在处理此仓库代码时提供指导。

## 项目概述

FluffOS 是一个 LPMUD 驱动程序，基于 MudOS 的最后一个版本 (v22.2b14)，拥有 10+ 年的错误修复和性能增强。它支持所有基于 LPC 的 MUD，只需最小的代码更改，并包括现代功能，如 UTF-8 支持、TLS、WebSocket 协议、异步 IO 和数据库集成。

## 架构

### 核心组件

**驱动层** (`src/`)

- `main.cc` - 驱动可执行文件的入口点
- `backend.cc` - 主游戏循环和事件处理
- `comm.cc` - 网络通信层
- `user.cc` - 用户/会话管理
- `symbol.cc` - 符号表管理
- `ofile.cc` - 对象文件处理

**VM 层** (`src/vm/`)

- `vm.cc` - 虚拟机初始化和管理
- `interpret.cc` - LPC 字节码解释器
- `simulate.cc` - 对象生命周期的模拟引擎
- `master.cc` - 主对象管理
- `simul_efun.cc` - 模拟外部函数

**编译器层** (`src/compiler/`)

- `compiler.cc` - LPC 到字节码编译器
- `grammar.y` - 语法定义 (Bison)
- `lex.cc` - 词法分析器
- `generate.cc` - 代码生成

**包** (`src/packages/`)

- 按功能组织的模块化功能 (async, db, crypto 等)
- 每个包有 `.spec` 文件定义可用函数
- 核心包：core, crypto, db, math, parser, sockets 等。

**网络** (`src/net/`)

- `telnet.cc` - Telnet 协议实现
- `websocket.cc` - WebSocket 支持用于 Web 客户端
- `tls.cc` - SSL/TLS 加密支持
- `msp.cc` - MUD 声音协议支持

### 构建系统

**CMake 配置** (`CMakeLists.txt`, `src/CMakeLists.txt`)

- 需要 CMake 3.22+
- C++17 和 C11 标准
- Jemalloc 用于内存管理 (推荐)
- ICU 用于 UTF-8 支持
- OpenSSL 用于加密功能

**构建选项** (关键标志)

- `MARCH_NATIVE=ON` (默认) - 为当前 CPU 优化
- `STATIC=ON/OFF` - 静态 vs 动态链接
- `USE_JEMALLOC=ON` - 使用 jemalloc 内存分配器
- `PACKAGE_*` - 启用/禁用特定包
- `ENABLE_SANITIZER=ON` - 启用地址清理器用于调试

## 构建命令

### 首选 Windows 输出工作流

在 Windows 上，规范的构建入口点是仓库根目录的 [`build.cmd`](/D:/code/fluffos/build.cmd)。

- 将 `build.cmd` 视为产生可运行 Windows 工件的主要方式。
- 将 [`build/dist`](/D:/code/fluffos/build/dist) 视为本地 Windows 构建唯一支持的发布/输出目录。
- 将 [`build/work`](/D:/code/fluffos/build/work) 和任何其他构建子目录视为内部中间状态。
- 当 `build/dist` 可用时，不要要求用户从临时构建目录运行可执行文件。
- 驱动相关交付物应分阶段放入 `build/dist`，目前包括：
  - `driver.exe`
  - `lpccp.exe`
  - 运行时 DLL 依赖
  - `run-driver.cmd`
  - `include/`、`std/` 和 `www/`

如果任务更改了 Windows 驱动的生产方式，请更新 `build.cmd` 和 dist 分阶段脚本，以便最终可运行工件仍位于 `build/dist`。

### 开发构建

```bash
# 标准开发构建
mkdir build && cd build
cmake ..
make -j$(nproc) install

# 带清理器的调试构建
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZER=ON
```

### 生产构建

```bash
# 生产就绪静态构建
cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC=ON -DMARCH_NATIVE=OFF
make install
```

### 包特定构建

```bash
# 构建时不带数据库支持
cmake .. -DPACKAGE_DB=OFF

# 构建时禁用特定包
cmake .. -DPACKAGE_CRYPTO=OFF -DPACKAGE_COMPRESS=OFF
```

## 测试

### 单元测试

```bash
# 运行所有测试 (需要 GTest)
cd build
make test

# 运行特定测试可执行文件
./src/lpc_tests
./src/ofile_tests
```

### LPC 测试

```bash
# 运行 LPC 测试套件
cd testsuite
../build/bin/driver etc/config.test -ftest
```

### 集成测试

```bash
# 使用测试配置运行驱动
./build/bin/driver testsuite/etc/config.test
```

## 持续集成

FluffOS 使用 GitHub Actions 在多个平台和配置上进行全面 CI/CD。

### CI 矩阵

**Ubuntu CI** (`.github/workflows/ci.yml`)

- **编译器**：GCC 和 Clang
- **构建类型**：Debug 和 RelWithDebInfo
- **平台**：ubuntu-22.04
- **关键功能**：SQLite 支持、GTest 集成
- **步骤**：安装依赖 → CMake 配置 → 构建 → 单元测试 → LPC 测试套件

**macOS CI** (`.github/workflows/ci-osx.yml`)

- **构建类型**：Debug 和 RelWithDebInfo
- **平台**：macos-14 (Apple Silicon)
- **环境变量**：
  - `OPENSSL_ROOT_DIR=/usr/local/opt/openssl`
  - `ICU_ROOT=/opt/homebrew/opt/icu4c`
- **依赖**：cmake, pkg-config, pcre, libgcrypt, openssl, jemalloc, icu4c, mysql, sqlite3, googletest

**Windows CI** (`.github/workflows/ci-windows.yml`)

- **构建类型**：Debug 和 RelWithDebInfo
- **平台**：windows-latest with MSYS2/MINGW64
- **构建选项**：
  - `-DMARCH_NATIVE=OFF` (为可移植性)
  - `-DPACKAGE_CRYPTO=OFF`
  - `-DPACKAGE_DB_MYSQL=""` (禁用)
  - `-DPACKAGE_DB_SQLITE=1`
- **依赖**：mingw-w64 工具链, cmake, zlib, pcre, icu, sqlite3, jemalloc, gtest

**清理器 CI** (`.github/workflows/ci-sanitizer.yml`)

- **目的**：内存安全和错误检测
- **编译器**：仅 Clang
- **构建类型**：Debug 和 RelWithDebInfo
- **特殊标志**：`-DENABLE_SANITIZER=ON`
- **额外依赖**：libdw-dev, libbz2-dev

**Docker CI** (`.github/workflows/docker-publish.yml`)

- **基础镜像**：Alpine 3.18
- **构建配置**：静态链接，使用 `-DSTATIC=ON -DMARCH_NATIVE=OFF`
- **注册表**：GitHub 容器注册表 (ghcr.io)
- **触发器**：推送到 master、版本标签 (v*.*)、拉取请求

**代码质量 CI**

- **CodeQL 分析** (`.github/workflows/codeql-analysis.yml`)：安全漏洞检测
- **Coverity 扫描** (`.github/workflows/coverity-scan.yml`)：静态分析 (每周计划 + 推送)

**文档 CI** (`.github/workflows/gh-pages.yml`)

- **框架**：VitePress
- **构建**：Node.js 20 with npm
- **部署**：GitHub Pages
- **路径**：`docs/` 目录

### 本地运行 CI 等效构建

**Ubuntu 调试构建 (GCC)**

```bash
export CC=gcc CXX=g++
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DPACKAGE_DB_SQLITE=2 ..
make -j$(nproc) install
make test
cd ../testsuite && ../build/bin/driver etc/config.test -ftest
```

**Ubuntu 带清理器 (Clang)**

```bash
export CC=clang CXX=clang++
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DPACKAGE_DB_SQLITE=2 -DENABLE_SANITIZER=ON ..
make -j$(nproc) install
make test
```

**macOS 构建**

```bash
mkdir build && cd build
OPENSSL_ROOT_DIR="/usr/local/opt/openssl" ICU_ROOT="/opt/homebrew/opt/icu4c" \
  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPACKAGE_DB_SQLITE=2 ..
make -j$(sysctl -n hw.ncpu) install
make test
```

**Windows 构建 (MSYS2/MINGW64)**

```bash
mkdir build && cd build
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug \
  -DMARCH_NATIVE=OFF -DPACKAGE_CRYPTO=OFF \
  -DPACKAGE_DB_MYSQL="" -DPACKAGE_DB_SQLITE=1 ..
make -j$(nproc) install
```

**Docker 构建 (静态)**

```bash
docker build -t fluffos:local .
# 或在 Alpine 容器中手动构建
cmake .. -DMARCH_NATIVE=OFF -DSTATIC=ON
make install
ldd bin/driver  # 应显示 "not a dynamic executable"
```

### 按平台的 CI 依赖

**Ubuntu/Debian**

```bash
sudo apt update
sudo apt install -y build-essential autoconf automake bison expect \
  libmysqlclient-dev libpcre3-dev libpq-dev libsqlite3-dev \
  libssl-dev libtool libz-dev telnet libgtest-dev libjemalloc-dev \
  libdw-dev libbz2-dev  # 用于清理器构建
```

**macOS (Homebrew)**

```bash
brew install cmake pkg-config pcre libgcrypt openssl jemalloc icu4c \
  mysql sqlite3 googletest
```

**Windows (MSYS2 - MINGW64 shell)**

```bash
pacman --noconfirm -S --needed \
  mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-zlib mingw-w64-x86_64-pcre \
  mingw-w64-x86_64-icu mingw-w64-x86_64-sqlite3 \
  mingw-w64-x86_64-jemalloc mingw-w64-x86_64-gtest \
  bison make
```

**Alpine (Docker/静态)**

```bash
apk add --no-cache linux-headers gcc g++ clang-dev make cmake bash \
  mariadb-dev mariadb-static postgresql-dev sqlite-dev sqlite-static \
  openssl-dev openssl-libs-static zlib-dev zlib-static icu-dev icu-static \
  pcre-dev bison git musl-dev libelf-static elfutils-dev \
  zstd-static bzip2-static xz-static
```

## 开发工作流

### GitHub / PR 安全规则

- 本仓库的 GitHub 日常操作默认只允许针对 fork 仓库 `lude2001/fluffos` 执行。
- 严禁代理向上游仓库 `fluffos/fluffos` 发起 Pull Request。
- 严禁代理把任何分支推送到上游仓库 `fluffos/fluffos`。
- 如果需要和上游仓库交互，必须先得到用户的明确、直接许可；没有明确许可时，一律视为禁止。
- 当代理使用 `gh`、`git push`、`gh pr create`、`gh pr edit`、`gh pr reopen` 等命令时，必须显式检查目标仓库是否为 `lude2001/fluffos`；如果不是，必须停止并报错，而不是继续执行。
- 默认工作流是不创建远程 Pull Request；除非用户明确要求，否则代理只应在本地分支完成开发、在本地合并回 `master`，然后仅把 `master` 推送到 `origin`。

### 代码生成

构建期间会自动生成几个源文件：

- `grammar.autogen.cc/.h` - 来自 `grammar.y` (Bison)
- `efuns.autogen.cc/.h` - 来自包规范
- `applies_table.autogen.cc/.h` - 来自应用定义
- `options.autogen.h` - 来自配置选项

### 添加新函数

1. 将函数添加到适当的包 `.spec` 文件
2. 在相应的 `.cc` 文件中实现函数
3. 运行构建以重新生成自动生成的文件
4. 在 `testsuite/` 目录中添加测试

### 调试

```bash
# 使用调试符号和清理器构建
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZER=ON

# 使用 GDB 运行
gdb --args ./build/bin/driver config.test

# 使用 Valgrind 进行内存调试
valgrind --leak-check=full ./build/bin/driver config.test
```

## 平台特定说明

### Ubuntu/Debian

```bash
# 安装依赖
sudo apt install build-essential bison libmysqlclient-dev libpcre3-dev \
  libpq-dev libsqlite3-dev libssl-dev libz-dev libjemalloc-dev libicu-dev \
  libgtest-dev  # 用于测试
```

### macOS

```bash
# 安装依赖
brew install cmake pkg-config mysql pcre libgcrypt openssl jemalloc icu4c \
  sqlite3 googletest  # 添加 sqlite3 和 googletest 用于测试

# 使用环境变量构建 (Apple Silicon)
OPENSSL_ROOT_DIR="/usr/local/opt/openssl" ICU_ROOT="/opt/homebrew/opt/icu4c" cmake ..
# 对于 Intel Mac，使用：
# OPENSSL_ROOT_DIR="/usr/local/opt/openssl" ICU_ROOT="/usr/local/opt/icu4c" cmake ..
```

### Windows (MSYS2)

```bash
# 安装 MSYS2 包 (在 MSYS2 shell 中运行，然后切换到 MINGW64 shell 进行构建)
pacman --noconfirm -S --needed \
  git mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-zlib mingw-w64-x86_64-pcre \
  mingw-w64-x86_64-icu mingw-w64-x86_64-sqlite3 \
  mingw-w64-x86_64-jemalloc mingw-w64-x86_64-gtest \
  bison make

# 注意：在 Windows 上通常禁用 PACKAGE_CRYPTO
# 在 MINGW64 终端中构建
cmake -G "MSYS Makefiles" -DMARCH_NATIVE=OFF \
  -DPACKAGE_CRYPTO=OFF -DPACKAGE_DB_MYSQL="" -DPACKAGE_DB_SQLITE=1 ..
```

## 关键目录

- `src/` - 主驱动源代码
- `src/packages/` - 模块化包实现
- `src/vm/` - 虚拟机和解释器
- `src/compiler/` - LPC 编译器
- `src/net/` - 网络协议实现
- `testsuite/` - LPC 测试程序和配置
- `docs/` - 文档 (Markdown 和 Jekyll)
- `build/` - 构建输出目录 (自动生成)

## 配置文件

- `Config.example` - 示例驱动配置
- `src/local_options` - 本地构建选项 (从 `local_options.README` 复制)
- `testsuite/etc/config.test` - 测试配置
- 包特定的 `.spec` 文件定义可用函数

## 常见开发任务

### 添加新包

1. 在 `src/packages/[package-name]/` 中创建目录
2. 添加 `CMakeLists.txt`、`.spec` 文件和源文件
3. 更新 `src/packages/CMakeLists.txt`
4. 在 `testsuite/` 中添加测试

### 修改编译器

1. 编辑 `src/compiler/grammar.y` 以进行语法更改
2. 如果可用，使用 Bison 重新生成语法
3. 更新相应的编译器组件

### 调试内存问题

1. 使用 `-DENABLE_SANITIZER=ON` 构建
2. 在 LPC 中使用 `mud_status()` efun 获取运行时内存信息
3. 检查 `testsuite/log/debug.log` 以获取详细日志
