# Windows LPC Runtime Installer Design

## Goal

为 FluffOS 增加一个面向 Windows 的可安装运行时发行方案，让用户可以像安装解释器一样安装 LPC 运行环境，并在安装后通过全局命令运行现有工具链。

第一阶段目标是交付一个 Windows 安装包，完成以下能力：

- 安装完整 LPC 运行时与工具链
- 对外暴露全局命令 `lpcprj` 与 `lpccp`
- 支持用户自定义安装目录
- 在安装完成后询问是否将命令加入 `PATH`
- 保持目录结构和命令边界对未来 Linux 发行兼容友好

## Background

当前仓库已经具备稳定的 Windows 分发 staging 流程：

- [`build.cmd`](/D:/code/fluffos/build.cmd) 负责在 Windows 上编译目标
- [`scripts/stage-driver-dist.ps1`](/D:/code/fluffos/scripts/stage-driver-dist.ps1) 将可运行工件整理到 `build/dist`

现有 `build/dist` 已经是一个自包含、可运行的目录，包含：

- `driver.exe`
- `lpccp.exe`
- 运行时 DLL
- `include/`
- `std/`
- `www/`
- `run-driver.cmd`

这说明 FluffOS 已经拥有“发行目录”的雏形。新的设计不应推翻当前构建流程，而应在 `build/dist` 之后增加“安装镜像”和“安装器”这一层，把当前的可运行目录转变成可安装的 LPC 运行时。

## Goals and Non-Goals

### Goals

- 第一阶段只实现 Windows 安装包
- 安装后可在任意终端运行 `lpcprj` 和 `lpccp`
- 不要求用户从临时构建目录直接运行二进制
- 安装布局不写死在 `Program Files`，必须支持用户自定义安装目录
- 为未来 Linux 版本预留兼容位，包括目录层级和命令命名

### Non-Goals

- 第一阶段不实现 Linux 安装包
- 第一阶段不做多版本管理器
- 第一阶段不携带 mudlib 模板工程
- 第一阶段不实现 `lpccall`
- 第一阶段不重命名或重构现有核心驱动内部二进制实现

## User-Facing Command Design

本设计采用“稳定对外命令名 + 内部实现二进制分层”的方式。

### Public commands

- `lpcprj`
  - 代表原先“启动完整驱动环境”的入口
  - 面向用户暴露，安装后进入全局命令空间
- `lpccp`
  - 保留现有命名
  - 继续作为 LPC 编译工具对外暴露
- `lpccall`
  - 本期不实现
  - 仅作为未来“运行某个类/函数”的预留命名

### Naming rationale

不将 `lpcrun` 作为第一阶段正式命令，是为了避免未来“运行完整 mudlib”与“运行单个类函数”在语义上冲突。

`lpcprj` 明确代表一个完整项目/完整运行时入口，未来若实现 `lpccall`，两者边界将保持清晰：

- `lpcprj` = 启动完整驱动 / 完整 mudlib 项目
- `lpccp` = 编译工具
- `lpccall` = 后续扩展的对象/函数调用入口

## Chosen Packaging Approach

### Option 1: Keep shipping only zip artifacts

继续输出 zip 包，让用户手动解压并自行添加到 `PATH`。

优点：

- 改动最小
- 复用现有 `build/dist`

缺点：

- 不是“安装解释器”体验
- 安装目录、PATH、卸载都需要用户手工处理
- 不利于后续形成稳定发行规范

### Option 2: Promote `build/dist` to release staging and generate an installer

把 `build/dist` 明确为标准 staging 目录，再从该目录生成 Windows 安装镜像和安装包。

优点：

- 延续当前构建入口和分发习惯
- 构建与安装分层清晰
- 更容易为未来 Linux 包管理方式复用相同安装布局

缺点：

- 需要新增安装镜像整理逻辑与安装脚本

### Option 3: Build a full version manager now

直接实现类似 Python launcher 或版本切换器的方案。

优点：

- 长期形态完整

缺点：

- 第一阶段明显过重
- 与当前“安装后全局运行某个 LPC 驱动环境”的目标不匹配

## Chosen Approach

采用 Option 2。

也就是：

1. 保持 [`build.cmd`](/D:/code/fluffos/build.cmd) 作为 Windows 构建入口
2. 保持 `build/dist` 作为可运行 staging 目录
3. 在 staging 之后增加“安装镜像整理”
4. 从安装镜像生成 Windows 安装包

这样可以在最小风险下，把当前“可运行目录”升级为“可安装运行时”。

## Installation Layout

安装目标目录必须允许用户自定义，因此布局设计不能依赖固定盘符或固定根目录。

推荐安装布局如下：

```text
<install-root>/
  bin/
    lpcprj.exe
    lpccp.exe
  libexec/
    fluffos/
      driver.exe
  share/
    fluffos/
      include/
      std/
      www/
  runtime/
    *.dll
  LICENSE
  README.txt
```

其中：

- `<install-root>` 默认值可以是 `C:\Program Files\FluffOS\`
- 但安装器必须允许改为任意目录，例如 `D:\Apps\FluffOS\`
- 所有工具都必须通过相对路径发现其配套资源，而不是依赖硬编码绝对路径

### Layout rationale

- `bin/` 用于承载用户直接调用的稳定命令
- `libexec/fluffos/` 用于承载内部实现二进制，便于未来替换内部执行方式而不影响用户入口
- `share/fluffos/` 用于放置平台无关资源，兼容未来 Linux 的常见目录语义
- `runtime/` 作为 Windows 第一阶段的 DLL 收纳层，后续可以根据实现细节调整为与二进制同目录放置，但逻辑上仍视为运行时私有依赖

## Command Launch Behavior

### `lpcprj`

`lpcprj` 应该是一个面向用户的稳定入口，而不是要求用户直接运行内部 `driver.exe`。

第一阶段建议将 `lpcprj.exe` 设计为一个薄包装层，职责包括：

- 定位自身安装根目录
- 设置运行时所需的 DLL 搜索路径
- 定位内部 `libexec/fluffos/driver.exe`
- 将用户参数原样转发给 `driver.exe`
- 保持现有驱动返回码

这样可以把内部实现细节封装在包装层之后，避免未来安装布局变化时影响用户脚本。

### `lpccp`

`lpccp` 保持现有命令名和主要行为不变。

第一阶段可采用两种实现方式之一：

- 直接将 `lpccp.exe` 放到 `bin/`
- 或像 `lpcprj` 一样通过包装层调用内部实现

若 `lpccp` 未来也需要稳定的资源定位或内部重构空间，则更推荐包装层方案；若第一阶段以最小改动为主，则可以直接把 `lpccp.exe` 放在 `bin/`。

本设计倾向于：

- `lpcprj.exe` 使用包装层
- `lpccp.exe` 第一阶段可直接暴露

以降低首期改造复杂度。

## Installer Behavior

安装器需要提供传统 Windows 安装体验，但安装布局和交互要尽量保持跨平台思路。

### Required installer flow

1. 用户启动安装包
2. 安装器显示默认安装目录
3. 用户可修改安装目录
4. 安装器复制运行时、工具链和资源文件
5. 安装完成后询问是否将 `bin/` 目录加入 `PATH`
6. 若用户同意，则更新 `PATH`
7. 若用户拒绝，安装仍然视为成功

### PATH behavior

用户明确要求“安装完成后再询问是否加入 PATH”，因此此行为属于设计约束，而不是可选优化。

建议第一阶段默认更新“用户级 PATH”，理由如下：

- 不必强依赖管理员权限
- 失败率更低
- 更适合开发者本机安装

如果安装器运行在管理员权限下，后续也可以扩展为允许用户选择“加入系统 PATH”，但这不是第一阶段必需项。

### Silent install compatibility

第一阶段可以不优先实现完整静默安装体验，但建议预留以下参数语义：

- 安装目录参数
- 是否加入 `PATH`

这样便于未来接入 CI、自动化部署或 Linux 包装脚本的参数对齐。

## Build and Release Flow

### Current baseline

当前 Windows 构建主线是：

1. [`build.cmd`](/D:/code/fluffos/build.cmd) 编译 `driver` 和 `lpccp`
2. [`scripts/stage-driver-dist.ps1`](/D:/code/fluffos/scripts/stage-driver-dist.ps1) 生成 `build/dist`

### Proposed flow

新增分发层后，Windows 发布流程应演化为：

1. `build.cmd`
2. 生成 `build/work`
3. 生成 `build/dist`
4. 从 `build/dist` 生成安装镜像目录，例如 `build/install-image`
5. 从安装镜像生成最终安装包，例如 `build/fluffos-<version>-windows-x86_64-installer.exe`

### Why not package directly from `build/work`

不建议直接从 `build/work` 打安装包，因为：

- `build/work` 属于内部构建状态
- 产物结构会随着 CMake 目标和 install 逻辑变化而变化
- `build/dist` 已经是团队当前认可的 Windows 运行时边界

因此，安装器应建立在 `build/dist` 之上，而不是绕开它。

## Compatibility with Future Linux Support

虽然第一阶段只做 Windows，但设计需要保证未来 Linux 版本能够沿用大部分命令与目录语义。

具体约束如下：

- `lpcprj` 与 `lpccp` 作为跨平台稳定命令名保留
- 资源目录采用 `share/fluffos/...` 语义
- 内部可执行文件采用 `libexec/fluffos/...` 语义
- 用户脚本不依赖 Windows 专属路径写法

未来 Linux 发行时，可以将同一逻辑映射到：

```text
/usr/bin/lpcprj
/usr/bin/lpccp
/usr/libexec/fluffos/driver
/usr/share/fluffos/include
/usr/share/fluffos/std
/usr/share/fluffos/www
```

这意味着 Windows 第一阶段的目录设计不是一次性方案，而是未来跨平台布局的预演。

## Implementation Outline

本设计不展开逐文件实现细节，但第一阶段大体需要以下工作：

1. 为 `lpcprj` 增加包装层可执行文件或等价启动器
2. 新增“安装镜像整理”脚本，把 `build/dist` 转换为稳定安装布局
3. 引入 Windows 安装器脚本或打包工程
4. 在安装器中实现“自定义目录”和“安装完成后询问是否加入 PATH”
5. 更新 release 流程，使 Windows 发布工件从 zip 逐步扩展为 installer
6. 更新文档，说明安装后如何使用 `lpcprj` 和 `lpccp`

## Risks

主要风险集中在路径和启动封装，而不是编译本身：

- `lpcprj` 包装层无法稳定找到内部 `driver.exe`
- DLL 搜索路径设置不一致，导致用户在自定义安装目录下运行失败
- `include/std/www` 相对路径假设与现有驱动行为不一致
- 继续复用 `lpccp.exe` 时，其资源定位方式与新布局不兼容
- PATH 写入逻辑在不同权限场景下行为不一致

## Success Criteria

满足以下条件时，视为第一阶段设计成功：

- Windows 上可生成一个可安装的 FluffOS 安装包
- 安装目录可由用户自定义
- 安装完成后明确询问是否加入 `PATH`
- 用户同意后，可在新终端中直接运行 `lpcprj` 与 `lpccp`
- 用户拒绝加入 `PATH` 时，安装仍然成功
- 现有 `build/dist` 仍保持为受支持的 Windows staging 边界
- 目录与命令语义为未来 Linux 发行保留兼容位

## Open Questions

以下问题不阻塞第一阶段设计落地，但在实现前需要定稿：

- `lpccp` 第一阶段是否直接暴露原始二进制，还是也包一层稳定包装器
- Windows 安装器技术栈选型使用哪一种方案最合适
- 是否在第一阶段继续同时发布 zip 与 installer 两种 Windows 工件
