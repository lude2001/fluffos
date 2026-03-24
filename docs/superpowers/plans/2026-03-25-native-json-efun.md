# Native JSON Efun Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 FluffOS 新增一个行为兼容当前 LPC `std/json` 的原生 JSON package，提供 `json_encode()` 和 `json_decode()` efun，并补齐兼容与性能回归测试。

**Architecture:** 通过新增 `PACKAGE_JSON` 把 JSON 编解码下沉到驱动层，用 C++ 递归完成 `svalue_t` 与 JSON 文本之间的转换，同时显式保留当前 mudlib 所依赖的兼容语义，如 `false/null -> 0`、非字符串 key 跳过和循环引用内部输出 `null`。测试层面先以现有 LPC `std/json` 为行为基准，再补充差分测试和性能基准，确保切换后 mudlib 服务不受影响。

**Tech Stack:** C++17, FluffOS package system, nlohmann/json, LPC testsuite, existing Windows/MSYS2 CMake workflow

---

## 文件结构

- 新建 `D:/code/fluffos/src/packages/json/CMakeLists.txt`
  - 注册 JSON package 源文件。
- 新建 `D:/code/fluffos/src/packages/json/json.spec`
  - 声明 `json_encode()` 和 `json_decode()` efun。
- 新建 `D:/code/fluffos/src/packages/json/json.cc`
  - 实现 JSON 编解码、兼容语义、循环引用处理。
- 修改 `D:/code/fluffos/src/packages/CMakeLists.txt`
  - 把新 package 挂到现有 package 构建链路中。
- 可能修改 `D:/code/fluffos/CMakeLists.txt`
  - 若仓库使用 `PACKAGE_*` 配置入口，需要补 `PACKAGE_JSON` 开关。
- 修改 `D:/code/fluffos/testsuite/single/tests/std/json.c`
  - 扩展兼容测试，覆盖更多行为边界。
- 修改 `D:/code/fluffos/testsuite/command/speed.c`
  - 补复杂嵌套 JSON 性能基准。
- 可选修改 `D:/code/fluffos/testsuite/single/simul_efun.c`
  - 在兼容确认后移除 `inherit "std/json";`，让 testsuite 直接走原生 efun。

---

## Chunk 1: Package scaffolding and failing compatibility tests

### Task 1: 建立原生 JSON package 骨架

**Files:**
- Create: `D:/code/fluffos/src/packages/json/CMakeLists.txt`
- Create: `D:/code/fluffos/src/packages/json/json.spec`
- Modify: `D:/code/fluffos/src/packages/CMakeLists.txt`
- Modify: `D:/code/fluffos/CMakeLists.txt`

- [ ] **Step 1: 写出 package 接入前的失败验证**

运行：

```powershell
cmake --build build --target driver
```

预期：

- 当前构建不包含 `PACKAGE_JSON`
- 还没有新的 JSON package 文件

- [ ] **Step 2: 新建 package 构建骨架**

新增：

- `src/packages/json/CMakeLists.txt`
- `src/packages/json/json.spec`

并在 package 总表中挂接。

- [ ] **Step 3: 给构建系统增加 `PACKAGE_JSON` 开关**

在仓库现有 `PACKAGE_*` 风格下补一项 JSON package 开关，默认开启。

- [ ] **Step 4: 重新生成并构建**

运行：

```powershell
cmake -S . -B build
cmake --build build --target driver
```

预期：

- `efuns.autogen.*` 正常生成
- driver 可编译，但 `json.cc` 仍可能因为函数未实现而失败

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/src/packages/json/CMakeLists.txt D:/code/fluffos/src/packages/json/json.spec D:/code/fluffos/src/packages/CMakeLists.txt D:/code/fluffos/CMakeLists.txt
git commit -m "build: add native json package scaffolding"
```

### Task 2: 先补行为兼容测试并确认它们会失败

**Files:**
- Modify: `D:/code/fluffos/testsuite/single/tests/std/json.c`

- [ ] **Step 1: 在 LPC testsuite 中增加新的兼容用例**

补以下测试：

- `json_decode("true") == 1`
- `json_decode("false") == 0`
- `json_decode("null") == 0`
- 非字符串 key 被跳过
- unsupported 值编码为 `null`
- 循环引用内部位置编码为 `null`
- 更复杂的嵌套结构 round-trip

- [ ] **Step 2: 运行测试，确认新增断言先失败**

运行：

```powershell
$env:PATH = "D:\code\fluffos\build\dist;" + $env:PATH
& D:\code\fluffos\build\dist\driver.exe etc/config.test -ftest
```

工作目录：

```text
D:\code\fluffos\testsuite
```

预期：

- 新增 JSON 测试因为原生 efun还未实现、或行为尚未覆盖而失败

- [ ] **Step 3: 提交**

```powershell
git add D:/code/fluffos/testsuite/single/tests/std/json.c
git commit -m "test: expand json compatibility coverage"
```

---

## Chunk 2: Native JSON decode path

### Task 3: 先实现最小 `json_decode()`，让基础解码测试转绿

**Files:**
- Create: `D:/code/fluffos/src/packages/json/json.cc`

- [ ] **Step 1: 在 `json.cc` 中先写出最小 decode 路径**

覆盖：

- object -> mapping
- array -> array
- string -> string
- integer -> int
- float / exponent -> float
- `true -> 1`
- `false -> 0`
- `null -> 0`

- [ ] **Step 2: 处理异常路径**

确保非法 JSON 输入时抛出稳定驱动错误，不留下半初始化 `svalue_t`。

- [ ] **Step 3: 重新构建 driver**

运行：

```powershell
cmake --build build --target driver
```

- [ ] **Step 4: 运行 JSON 测试，确认基础 decode 用例通过**

运行：

```powershell
$env:PATH = "D:\code\fluffos\build\dist;" + $env:PATH
& D:\code\fluffos\build\dist\driver.exe etc/config.test -ftest
```

工作目录：

```text
D:\code\fluffos\testsuite
```

预期：

- 基础 decode 测试转绿
- encode 相关兼容测试仍可能失败

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/src/packages/json/json.cc
git commit -m "feat: add native json decode support"
```

### Task 4: 补齐 Unicode 和 surrogate pair 解码兼容

**Files:**
- Modify: `D:/code/fluffos/src/packages/json/json.cc`
- Modify: `D:/code/fluffos/testsuite/single/tests/std/json.c`

- [ ] **Step 1: 先补更严格的 Unicode 测试**

增加：

- BMP 字符
- surrogate pair
- `\u001b` 往返
- 错误 surrogate pair 输入

- [ ] **Step 2: 运行测试并确认新增用例先失败**

运行与上一任务相同的 testsuite 命令。

- [ ] **Step 3: 在 decode 路径中修正 Unicode 兼容行为**

保证与当前 LPC 版使用场景一致，而不是仅依赖默认库行为。

- [ ] **Step 4: 重新运行 JSON 测试**

预期：

- Unicode 相关测试全部通过

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/src/packages/json/json.cc D:/code/fluffos/testsuite/single/tests/std/json.c
git commit -m "feat: match native json unicode decoding behavior"
```

---

## Chunk 3: Native JSON encode path

### Task 5: 实现基础 `json_encode()`，覆盖数组、mapping 和字符串转义

**Files:**
- Modify: `D:/code/fluffos/src/packages/json/json.cc`

- [ ] **Step 1: 在测试中先确保 encode 目标行为已经被覆盖**

重点检查 testsuite 是否已经覆盖：

- 基础数组编码
- mapping 编码
- 字符串转义
- `undefined -> "null"`

如果缺失，先补测试再继续。

- [ ] **Step 2: 实现最小 encode 路径**

覆盖：

- `undefined`
- `int`
- `float`
- `string`
- `array`
- `mapping`

- [ ] **Step 3: 保持当前 key 过滤语义**

非字符串 mapping key 直接跳过，不抛错、不自动字符串化。

- [ ] **Step 4: 重新构建并运行 testsuite**

运行：

```powershell
cmake --build build --target driver
$env:PATH = "D:\code\fluffos\build\dist;" + $env:PATH
& D:\code\fluffos\build\dist\driver.exe etc/config.test -ftest
```

工作目录：

```text
D:\code\fluffos\testsuite
```

预期：

- 基础 encode/decode 测试通过

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/src/packages/json/json.cc
git commit -m "feat: add native json encode support"
```

### Task 6: 实现循环引用与 unsupported 值的兼容行为

**Files:**
- Modify: `D:/code/fluffos/src/packages/json/json.cc`
- Modify: `D:/code/fluffos/testsuite/single/tests/std/json.c`

- [ ] **Step 1: 先补失败测试**

增加：

- 自引用 array
- 自引用 mapping
- 包含 unsupported 值的数组和 mapping

预期行为：

- 递归内部位置输出 `null`
- 不支持类型输出 `null`

- [ ] **Step 2: 运行 testsuite，确认这些断言先失败**

运行与上一任务相同。

- [ ] **Step 3: 在 `json.cc` 中增加递归访问跟踪**

至少跟踪：

- `array_t *`
- `mapping_t *`

确保只把“内部递归位置”编码成 `null`，而不是整个顶层对象直接失败。

- [ ] **Step 4: 再次运行 testsuite**

预期：

- 循环引用和 unsupported 值兼容测试通过

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/src/packages/json/json.cc D:/code/fluffos/testsuite/single/tests/std/json.c
git commit -m "feat: preserve json circular reference compatibility"
```

---

## Chunk 4: Switch testsuite to the native efun and verify rollout safety

### Task 7: 让 testsuite 直接使用原生 efun

**Files:**
- Modify: `D:/code/fluffos/testsuite/single/simul_efun.c`

- [ ] **Step 1: 先确认所有 JSON 兼容测试都已经通过**

运行：

```powershell
$env:PATH = "D:\code\fluffos\build\dist;" + $env:PATH
& D:\code\fluffos\build\dist\driver.exe etc/config.test -ftest
```

工作目录：

```text
D:\code\fluffos\testsuite
```

- [ ] **Step 2: 从 `simul_efun.c` 中移除 `inherit "std/json";`**

让 testsuite 改为直接依赖驱动原生 `json_encode/json_decode`。

- [ ] **Step 3: 再次运行 testsuite**

预期：

- 现有 JSON 相关服务与测试仍通过
- 不再依赖 mudlib sefun

- [ ] **Step 4: 提交**

```powershell
git add D:/code/fluffos/testsuite/single/simul_efun.c
git commit -m "test: switch testsuite to native json efuns"
```

---

## Chunk 5: Performance validation and final cleanup

### Task 8: 为复杂嵌套结构补性能基准

**Files:**
- Modify: `D:/code/fluffos/testsuite/command/speed.c`

- [ ] **Step 1: 在 `speed.c` 中新增复杂嵌套 mapping/array 基准**

至少补一组：

- 中等嵌套
- 深层嵌套
- 大量节点

- [ ] **Step 2: 先构建并运行测速**

运行：

```powershell
$env:PATH = "D:\code\fluffos\build\dist;" + $env:PATH
& D:\code\fluffos\build\dist\driver.exe etc/config.test -fspeed
```

工作目录：

```text
D:\code\fluffos\testsuite
```

- [ ] **Step 3: 记录原生实现与旧 LPC 基线的差距**

把至少这几项写进开发记录：

- `test.json` encode/decode
- 复杂嵌套 encode
- 复杂嵌套 decode

- [ ] **Step 4: 提交**

```powershell
git add D:/code/fluffos/testsuite/command/speed.c
git commit -m "perf: add nested json benchmarks"
```

### Task 9: 做最终回归验证

**Files:**
- Verify: `D:/code/fluffos/src/packages/json/json.cc`
- Verify: `D:/code/fluffos/testsuite/single/tests/std/json.c`
- Verify: `D:/code/fluffos/testsuite/command/speed.c`

- [ ] **Step 1: 完整重配并构建**

运行：

```powershell
cmake -S . -B build
cmake --build build --target driver
```

- [ ] **Step 2: 跑 JSON 相关 testsuite**

运行：

```powershell
$env:PATH = "D:\code\fluffos\build\dist;" + $env:PATH
& D:\code\fluffos\build\dist\driver.exe etc/config.test -ftest
```

工作目录：

```text
D:\code\fluffos\testsuite
```

- [ ] **Step 3: 跑性能基准**

运行：

```powershell
$env:PATH = "D:\code\fluffos\build\dist;" + $env:PATH
& D:\code\fluffos\build\dist\driver.exe etc/config.test -fspeed
```

工作目录：

```text
D:\code\fluffos\testsuite
```

- [ ] **Step 4: 检查工作树变更**

运行：

```powershell
git status --short
```

确认只包含预期文件。

- [ ] **Step 5: 提交**

```powershell
git add D:/code/fluffos/src/packages/json/json.cc D:/code/fluffos/src/packages/json/json.spec D:/code/fluffos/src/packages/json/CMakeLists.txt D:/code/fluffos/src/packages/CMakeLists.txt D:/code/fluffos/CMakeLists.txt D:/code/fluffos/testsuite/single/tests/std/json.c D:/code/fluffos/testsuite/single/simul_efun.c D:/code/fluffos/testsuite/command/speed.c
git commit -m "feat: add native json efuns"
```

Plan complete and saved to `docs/superpowers/plans/2026-03-25-native-json-efun.md`. Ready to execute?
