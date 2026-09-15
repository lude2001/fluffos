# FluffOS — lude2001 distribution

[简体中文](README_CN.md)

This repository is an independently maintained FluffOS distribution used
primarily by our own LPC game projects, including Jianghu Yingjiezhuan. Its
source is public so that compatible LPC mudlibs and related tools can reuse it,
but general-purpose downstream support is secondary to the stability and
requirements of our production stack.

This is **not** the official FluffOS repository.

## Repository relationship

| Role | Repository |
| --- | --- |
| Official upstream | [`fluffos/fluffos`](https://github.com/fluffos/fluffos) |
| This distribution | [`lude2001/fluffos`](https://github.com/lude2001/fluffos) |
| Official documentation | [fluffos.info](https://www.fluffos.info/) |

The current codebase was re-established on the official `v2026.0901.0` stable
release. Official releases are reviewed read-only and adopted deliberately;
this repository does not continuously mirror upstream `master`.

See the [upstream merge tracker](docs/superpowers/upstream-merge-tracker.md) for
the exact upstream snapshot, retained local capabilities, validation evidence,
and intentionally excluded changes. The
[migration plan](docs/superpowers/plans/2026-09-15-official-stable-rebase-migration.md)
records the rebaseline and rollback boundary.

## What this distribution keeps

The driver remains compatible with ordinary FluffOS/MudOS-style LPC mudlibs.
Our maintained additions are intentionally kept at package, tool, or narrow
integration boundaries:

- native `json_encode()`, `json_decode()`, and `json_format()` efuns in
  `src/packages/json/`;
- HTTP helpers and streaming request/response parsers in
  `src/packages/lude_http/`;
- the opt-in runtime compile service and `lpccp` client in
  `src/extensions/compile_service/`;
- the Windows `build/dist` layout, `lpcprj`, installer, and staging scripts;
- four lightweight CI/release workflows maintained for this repository.

Features already supplied by the official stable release use the official
implementation wherever possible. Local changes to compiler or VM internals
must remain small, tested, and justified by an actual consumer or reproducible
compatibility issue.

## Build

### Windows

The supported Windows entry point is:

```powershell
.\build.cmd
```

Run it from the repository root with MSYS2/MinGW64 installed. Supported runtime
artifacts are staged only in `build/dist`, including `driver.exe`, `lpccp.exe`,
`lpcprj.exe`, required DLLs, headers, standard LPC files, and launch scripts.

`build.cmd` enables the local compile-service extension. In a custom CMake
build it is disabled by default and can be enabled explicitly with:

```text
-DENABLE_LUDE_COMPILE_SERVICE=ON
```

### Linux and macOS

Use the standard CMake flow:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMARCH_NATIVE=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Platform dependencies and additional build targets are documented by the
[official FluffOS project](https://github.com/fluffos/fluffos) and in
[`AGENTS.md`](AGENTS.md). Repository CI definitions are the authoritative
reference for the configurations we maintain.

## Test

Run binary-level tests with CTest:

```bash
ctest --test-dir build --output-on-failure
```

Run the LPC testsuite against an installed Unix build:

```bash
cd testsuite
../build/bin/driver etc/config.test -ftest
```

For the canonical Windows build:

```powershell
cd testsuite
..\build\dist\driver.exe etc\config.test -ftest
```

Changes affecting efuns, the compiler, VM state, persistence, networking, or
database behavior must include focused regression coverage. Passing a build
alone is not sufficient.

## Important paths

- `src/` — driver, compiler, VM, networking, and package source.
- `src/packages/` — modular efun packages.
- `src/extensions/` — optional local integrations.
- `testsuite/` — C++/LPC integration and regression coverage.
- `docs/` — driver and LPC documentation.
- `.github/workflows/` — this distribution's lightweight CI and artifacts.
- `build/dist/` — supported local Windows runtime output.

## Upstream updates

When adopting a newer official release:

1. inspect `fluffos/fluffos` read-only and pin an official stable tag;
2. validate the unmodified official baseline first;
3. reapply only the local packages, tools, workflows, and minimal hooks that
   still have a real consumer;
4. run the full driver, LPC, mudlib, and persistence compatibility checks;
5. update the [upstream merge tracker](docs/superpowers/upstream-merge-tracker.md)
   in the same change set.

Do not treat this repository as an automatic fork mirror, and do not send this
distribution's branches or release operations to the official repository.

## Scope and copyright

Issues and changes are evaluated against our own production requirements first.
Compatibility improvements useful to other LPC projects are welcome when they
do not weaken that boundary, but this repository does not replace official
FluffOS support channels.

Use and redistribution are governed by the notices in [`Copyright`](Copyright)
and by the licenses of bundled third-party components.
