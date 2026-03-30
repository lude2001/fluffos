---
layout: doc
title: cli / lpcprj
---

# cli / lpcprj

`lpcprj` 是 FluffOS 的项目/运行时启动器。

它是面向 Windows 安装布局的稳定入口，负责找到内部 `driver.exe`、设置运行时依赖环境、切换到配置文件所在目录，然后把启动请求转发给真正的 driver。

## 用法

```bash
lpcprj <config-file> [driver-args...]
```

其中：

- `<config-file>` 是要启动的 FluffOS 配置文件
- `[driver-args...]` 会原样继续传给内部 `driver.exe`

例如：

```powershell
lpcprj C:\mud\etc\config.cfg
```

带额外参数：

```powershell
lpcprj C:\mud\etc\config.cfg -dconnections -ftest --tracing trace.json
```

## 启动行为

当前实现有几个关键特征：

1. 仅在 Windows 上实现
2. 会把 `<config-file>` 先规范化成绝对路径
3. 会把当前工作目录切换到 `<config-file>` 所在目录
4. 会从 `lpcprj.exe` 自身位置反推出安装根目录，并查找：
   - `libexec/fluffos/driver.exe`
   - `runtime/`
5. 会把 `runtime/` 和 `driver.exe` 所在目录预置到 `PATH`
6. 最终启动内部 `driver.exe`，并返回 driver 的退出码

这意味着 `lpcprj` 适合“启动一个完整 LPC 项目/完整运行时”，而不是用来做独立离线编译。

如果你需要连接运行中的 driver 做重新编译，请改用 [`lpccp`](lpccp.html)。

## 适用布局

`lpcprj` 面向的是安装器或安装镜像布局。典型目录结构类似：

```text
<install-root>/
  bin/lpcprj.exe
  bin/lpccp.exe
  libexec/fluffos/driver.exe
  runtime/*.dll
  share/fluffos/include/
  share/fluffos/std/
  share/fluffos/www/
```

如果你是在仓库里做本地 Windows 开发，请注意：

- `build/dist` 是本地构建产物目录
- 当前本地开发场景更适合直接使用 `build/dist/driver.exe`
- 或者使用 `build/dist/run-driver.cmd`

不要假设 `build/dist/lpcprj.exe` 与安装布局完全等价，`lpcprj` 运行时需要的内部目录结构是按安装镜像设计的。

## 常见场景

### 安装后启动项目

安装器把 `bin/` 加入 `PATH` 后，可以直接在任意终端中运行：

```powershell
lpcprj C:\path\to\your\config.cfg
```

### 从非配置目录启动

即使当前终端不在 mudlib 目录下，`lpcprj` 仍会切换到配置文件目录再启动：

```powershell
cd C:\Users\me
lpcprj D:\games\my-mud\etc\config.cfg
```

这正是它和直接双击或直接调用内部 `driver.exe` 相比更稳定的地方之一。

## 标准错误输出

启动器级错误会输出到标准错误。常见形式包括：

```text
Usage: lpcprj <config-file> [driver-args...]
```

```text
Unable to find driver executable at ...
```

```text
Unable to find config file at ...
```

```text
Failed to launch driver.exe: ...
```

## 退出码

- `0`：内部 `driver.exe` 成功退出
- `1`：参数不足、配置文件不存在，或内部 `driver.exe` 返回 `1`
- `2`：启动器级错误，例如：
  - 当前平台不是 Windows
  - 找不到内部 `driver.exe`
  - `CreateProcessW` 启动失败
  - 启动器内部异常

## 注意事项

- `lpcprj` 不是独立编译器，它启动的是完整 driver
- 它比直接运行内部 `driver.exe` 更适合作为最终用户入口
- 对本地开发构建产物，优先区分：
  - 安装/发布入口：`lpcprj`
  - 仓库内本地 dist 入口：`driver.exe` 或 `run-driver.cmd`
