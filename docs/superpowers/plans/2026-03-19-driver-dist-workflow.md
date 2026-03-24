# Driver Dist Workflow Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a root `build.cmd` that builds the Windows driver and stages a runnable distribution under `build/dist`, including a colocated `run-driver.cmd`.

**Architecture:** `build.cmd` is the user-facing entry point and delegates file staging to a PowerShell helper for easier path handling and recursive DLL discovery. CMake builds into `build/work` so the generated files stay isolated from the published `build/dist` directory, which is refreshed in-place without deleting unrelated root files such as user-provided config files.

**Tech Stack:** Windows batch, PowerShell, MSYS2 `bash`, CMake, MinGW `objdump`

---

### Task 1: Add the build entry point

**Files:**
- Create: `D:/code/fluffos/build.cmd`

- [ ] Resolve repository, build, dist, and MSYS2 paths.
- [ ] Configure CMake with the verified Windows MySQL flags.
- [ ] Build the `driver` target.
- [ ] Call the staging helper after a successful build.

### Task 2: Add dist staging logic

**Files:**
- Create: `D:/code/fluffos/scripts/stage-driver-dist.ps1`

- [ ] Copy `build/work/src/driver.exe` into `build/dist`.
- [ ] Recursively discover non-system PE DLL dependencies with `objdump`.
- [ ] Copy required DLLs from `MSYS2/mingw64/bin`.
- [ ] Sync `testsuite/std`, `src/include`, and `src/www` into `build/dist`.
- [ ] Generate `build/dist/run-driver.cmd`.

### Task 3: Verify the workflow

**Files:**
- Verify: `D:/code/fluffos/build.cmd`
- Verify: `D:/code/fluffos/build/dist/run-driver.cmd`

- [ ] Run `build.cmd`.
- [ ] Confirm `build/dist/driver.exe` exists.
- [ ] Confirm runtime DLLs were staged into `build/dist`.
- [ ] Confirm `build/dist/run-driver.cmd` exists and prints usage when invoked without arguments.
