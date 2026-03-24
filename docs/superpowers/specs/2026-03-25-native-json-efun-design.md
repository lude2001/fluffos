# Native JSON Efun Design

## Goal

在驱动中新增原生 `json_encode()` 和 `json_decode()` efun，用 C++ 实现替代当前 testsuite mudlib 中的 LPC `std/json` sefun，同时保持现有 mudlib 所依赖的行为语义，避免以 JSON 为主的客户端协议服务在切换后出现业务回归。

## Background

当前 testsuite 中的 JSON 能力来自 [`testsuite/std/json.c`](/D:/code/fluffos/testsuite/std/json.c)，并通过 [`testsuite/single/simul_efun.c`](/D:/code/fluffos/testsuite/single/simul_efun.c#L8) 继承暴露给 mudlib。

这份实现的优点是简单直接、跨驱动风格兼容，但在 FluffOS 场景里有两个明显问题：

- 它完全运行在 LPC 层，复杂嵌套结构会消耗大量解释器递归、字符串拼接和中间对象分配。
- 它已经形成了一套 mudlib 依赖的“行为语义”，不能直接用标准 JSON 库默认行为替换。

对以 JSON 为主要协议载体的 mudlib 来说，这条路径已经属于性能关键路径，值得下沉到驱动原生层。

## Current Behavior Baseline

原生实现必须以当前 LPC 版 `std/json` 的行为为基准，而不是仅以“标准 JSON”作为基准。

当前实现中的关键语义包括：

- `json_decode("true")` 返回 `1`
- `json_decode("false")` 返回 `0`
- `json_decode("null")` 返回 `0`
- `json_encode(undefined)` 返回 `"null"`
- 非字符串 mapping key 在编码时会被跳过
- 无法表示为 JSON 的 LPC 值会编码成 `null`
- 循环引用不会报错，而是把内部递归位置编码成 `null`
- `\uXXXX`、UTF-16 surrogate pair、`\x1b` 这些特殊字符和转义都需要正确处理

用户已经确认字段顺序不要求一致，因此顺序不是兼容目标的一部分；真正需要保证的是行为兼容、协议稳定和内部逻辑正确。

## Non-Goals

- 不追求逐字符复刻当前 LPC 版的错误文案
- 不保证 mapping 字段输出顺序一致
- 不在这一轮引入 pretty-print、streaming parser 或增量解析 API
- 不在第一版中扩展为 BSON、CBOR、MessagePack 等其他格式
- 不把对象、function、class 等原本不稳定或不可 JSON 化的 LPC 值强行定义成新格式

## Options Considered

### Option 1: Keep JSON in mudlib LPC

继续沿用 [`testsuite/std/json.c`](/D:/code/fluffos/testsuite/std/json.c) 作为唯一实现。

优点：

- 零驱动改动
- 行为天然兼容

缺点：

- 性能瓶颈保留在热路径上
- 复杂嵌套结构的编码和解码成本仍由解释器承担
- 无法利用驱动内部已有的 C++ JSON 能力

### Option 2: Add native efuns directly to `core`

把 `json_encode/json_decode` 直接放进 `core` 包。

优点：

- 最直接，默认可用
- mudlib 切换成本低

缺点：

- JSON 语义直接绑定到核心层，边界不清晰
- 后续维护、开关控制、平台裁剪都不够灵活

### Option 3: Add a dedicated `PACKAGE_JSON`

新增独立 JSON package，导出原生 `json_encode/json_decode` efun。

优点：

- 与现有 package 体系一致
- 开关、构建、维护边界清晰
- 以后如果需要扩展 JSON 相关 helper，也有明确归属

缺点：

- 需要补一层 package 接线
- 第一版接入文件比直接改 `core` 多一点

## Chosen Approach

采用 Option 3：新增独立 `PACKAGE_JSON`，提供原生 `json_encode()` 和 `json_decode()` efun。

这个方案最适合当前仓库的架构，也最适合“保持 mudlib 行为兼容，同时提升协议热路径性能”的目标。

## Architecture

### Package layout

新增一个独立 package：

- `src/packages/json/CMakeLists.txt`
- `src/packages/json/json.spec`
- `src/packages/json/json.cc`

并更新：

- `src/packages/CMakeLists.txt`
- 相关自动生成链路，让 efun 正常进入 `efuns.autogen.*`

### Data conversion boundary

实现分成两部分：

1. `svalue_t -> JSON text`
2. `JSON text -> svalue_t`

虽然驱动内部已经在多个地方使用 `nlohmann::json`，例如 [`src/ofile.cc`](/D:/code/fluffos/src/ofile.cc#L143)，但对象文件 JSON 使用的是带 `"type"` / `"value"` 的内部格式，不适合直接作为 mudlib 协议 JSON 使用。

因此本设计不复用 `OFile` 的输出格式，只复用它的几个思路：

- 递归遍历 `svalue_t`
- 类型分派集中处理
- 在异常路径上认真管理 LPC 值的分配和释放

### Encoding semantics

原生 `json_encode(mixed value)` 的行为定义：

- `undefined` 编码为 `"null"`
- `int` 与 `float` 直接编码为 JSON number
- `string` 编码为 JSON string，保留当前转义处理
- `array` 编码为 JSON array
- `mapping` 编码为 JSON object
- 非字符串 mapping key 被跳过
- 不支持类型编码为 `null`
- 循环引用内部递归位置编码为 `null`

循环引用检测需要在 C++ 层维护一个“访问中容器集合”，至少覆盖 array 与 mapping。

### Decoding semantics

原生 `json_decode(string text)` 的行为定义：

- JSON object -> LPC mapping
- JSON array -> LPC array
- JSON string -> LPC string
- JSON integer -> LPC int
- JSON float / exponent -> LPC float
- JSON `true` -> `1`
- JSON `false` -> `0`
- JSON `null` -> `0`

解码错误不要求与 LPC 版报错文本逐字一致，但必须：

- 对非法 JSON 给出稳定错误
- 不产生内存泄漏
- 不返回半初始化的 LPC 值

## Compatibility Contract

为确保现有 mudlib 服务不受影响，第一版兼容契约定义如下：

- 行为兼容优先于标准 JSON 库默认语义
- 字段顺序不在兼容范围内
- 错误文案不要求逐字一致，但错误类型和失败时机要合理稳定
- testsuite 中现有 `std/json` 测试必须继续通过
- 需要新增一组“旧 LPC 实现 vs 新原生 efun”的差分测试，覆盖边界行为

## Testing Strategy

### Existing baseline tests

保留并扩展现有测试：

- [`testsuite/single/tests/std/json.c`](/D:/code/fluffos/testsuite/single/tests/std/json.c)

这份测试现在覆盖了：

- `0`、`0e0`、`0.000`
- 数组解码
- Unicode
- surrogate pair
- 基本 encode
- `\e` / `\x1b`

### New compatibility tests

新增测试覆盖：

- `true` / `false` / `null`
- 非字符串 mapping key 跳过
- 复杂嵌套 array/mapping
- 循环引用编码为 `null`
- unsupported 类型编码为 `null`
- 非法 JSON 输入失败
- 大量嵌套结构的 round-trip 稳定性

### Performance tests

扩展 [`testsuite/command/speed.c`](/D:/code/fluffos/testsuite/command/speed.c#L265)：

- 保留现有 `test.json` / `test2.json` 基线
- 新增复杂嵌套 mapping/array 性能测试
- 对比旧 LPC 版和原生版的 encode/decode 成本

## Rollout Strategy

为降低切换风险，采用分阶段切换：

1. 先实现原生 package 和 efun
2. testsuite 增加差分测试
3. 验证新实现与现有 LPC 版行为兼容
4. 再决定是否让 testsuite 的 `simul_efun` 移除 `inherit "std/json"`

这样做可以在不影响现有 mudlib 服务的前提下，把行为回归风险控制在 testsuite 阶段。

## Risks

主要风险不是实现难度，而是兼容语义偏移：

- 把 `false` 或 `null` 错解成新的 LPC 语义
- 循环引用处理从“输出 null”变成“直接异常”
- Unicode / surrogate pair 细节与现有 mudlib 行为不一致
- 递归解码时异常路径释放不完整
- mapping 顺序测试写得过于严格，误判行为回归

## Success Criteria

以下条件同时满足，视为设计成功：

- 原生 `json_encode/json_decode` 可通过 testsuite
- 与现有 LPC `std/json` 的行为差分测试通过
- 现有 mudlib JSON 服务无须改业务代码即可切换
- 复杂嵌套结构的性能相较 LPC 版有明显提升
- 方案边界清晰，后续可继续在 `PACKAGE_JSON` 中演进
