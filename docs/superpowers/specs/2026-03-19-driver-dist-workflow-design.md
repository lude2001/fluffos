# Driver Dist Workflow Design

**Goal:** Add a repeatable Windows build workflow that compiles FluffOS with MySQL enabled, stages a runnable distribution in `build/dist`, and provides a colocated `run-driver.cmd`.

**Layout:**
- `build.cmd` lives at the repository root and is the main entry point.
- `build/work` is the internal CMake build directory.
- `build/dist` is the runnable publish directory.
- `build/dist/run-driver.cmd` lives next to `driver.exe` and launches from the dist directory so relative config paths like `config.hell` resolve there.

**Build approach:**
- Reuse the known-good MSYS2 `MINGW64` CMake configuration already verified in this workspace.
- Keep the current Windows flags: `MSYS Makefiles`, `PACKAGE_CRYPTO=OFF`, MySQL enabled, MariaDB headers/libs from MSYS2, and extra link libraries for `libcurl` and `libzstd`.
- Keep the compile/build step separate from dist staging so the staging logic stays simple and inspectable.

**Dist staging:**
- Copy `build/work/src/driver.exe` into `build/dist`.
- Recursively inspect PE dependencies with `objdump` and copy non-system DLLs from `MSYS2/mingw64/bin` into `build/dist`.
- Sync runtime support directories needed by the driver install layout:
  - `testsuite/std` -> `build/dist/std`
  - `src/include` -> `build/dist/include`
  - `src/www` -> `build/dist/www`
- Generate `build/dist/run-driver.cmd` during staging so the runnable entry point always matches the latest dist layout.

**Run behavior:**
- `run-driver.cmd` changes working directory to its own folder before launching `driver.exe`.
- It forwards all arguments unchanged, so `run-driver.cmd config.hell` works as long as `config.hell` is in `build/dist` or an absolute path is provided.

**Assumptions:**
- MSYS2 is installed locally and available either via `MSYS2_ROOT` or standard locations such as `D:\software\msys2` or `C:\msys64`.
- The user manages their own config file placement; the dist sync does not delete unrelated root-level files in `build/dist`.
