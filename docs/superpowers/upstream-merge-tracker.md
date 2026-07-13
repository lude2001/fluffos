# Official FluffOS Merge Tracker

This document tracks read-only reviews of the official `fluffos/fluffos`
repository and records which upstream features were merged into this independent
fork.

Every future merge or backport of official functionality must update this file
in the same change set. Do not add an `upstream` git remote for this workflow;
official repository access stays read-only.

## Current Upstream Snapshot

- Official repository: `fluffos/fluffos`
- Official branch reviewed: `master`
- Latest official commit reviewed: `6b6f1699525c8c6b3b7c8d50c02003d85f33f217`
- Latest official commit title: `lpc-syntax: wire formatter into vscode extension, fix tokenizer/formatter bugs (#1259)`
- Official commit date: `2026-07-12T19:42:14Z`
- Latest local merge commit: `726e990d17b11614ac9387c4b60cdc7f77bf9d73`
  (`merge partial upstream audit safety fixes`)
- Previous local merge commit:
  `9ddb8d623bfe972f64d7681400ce498b01199932`
  (`merge upstream buffer byte semantics`)
- Review date: `2026-07-13`

## Merged In `c20b15e4`

The following official changes were selectively merged or manually backported:

- `request_clean_up()` efun from official issue-fix work.
- `set_clean_up()` efun from official cleanup scheduling work.
- `get_os_env()` and `set_os_env()` efuns with explicit read/write config
  allow-lists.
- `member_array()` flag 4 predicate mode and the reverse-search not-found fix.
- `call_out` handle lookup/removal fixes for older pending handles, including
  the union-owner guard.
- `pcre_match_all()` zero-width match handling to avoid infinite loops.
- Prompt-path error containment and null error-context guard.
- SQLite failed-execution cleanup to avoid double `sqlite3_finalize()`.
- `sprintf` column-mode wrapping fix that preserves indentation.
- Documentation and tests for the merged efuns and bug fixes.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check`

Note: the SQLite regression in `testsuite/single/tests/efuns/db.c` is present,
but the Windows build used for this validation skipped the SQLite section
because `__USE_SQLITE3__` was not enabled.

## Merged In `c168e3aa07dcfbd193485a51a7387ce055bf7412`

The following additional official fixes were selectively merged or manually
backported:

- PR #1256: `restore_variable()` now bounds true recursive nesting instead of
  cumulative container count, so wide-but-shallow arrays/mappings restore
  correctly.
- PR #1258, partially: compiler local-name allocation now handles identifiers
  larger than the normal 4096-byte block without overrunning it.
- PR #1258, partially: `restore_variable()` size-cache growth now detects
  signed integer overflow and fails the pre-pass cleanly.
- PR #1244, partially: `call_stack(4)` consistently returns `file:line` for
  every frame.
- PR #1244, partially: arithmetic compound assignment now keeps float semantics
  for declared `float` lvalues and promotes runtime `int op= float` results to
  float.
- PR #1238, partially: `unique_mapping()` now releases copied keys and partial
  mappings when callback or mapping construction paths unwind through `error()`.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- src/compiler/internal/grammar.autogen.cc src/compiler/internal/grammar.y src/compiler/internal/lex.cc src/packages/core/efuns_main.cc src/packages/ops/ops.cc src/vm/internal/base/interpret.cc src/vm/internal/base/mapping.cc src/vm/internal/base/object.cc testsuite/single/tests/efuns/call_stack.c testsuite/single/tests/efuns/restore_variable.c testsuite/single/tests/operators/compound_assign_float.c`

Notes:

- The Windows build regenerated `src/compiler/internal/grammar.autogen.cc` from
  `src/compiler/internal/grammar.y`; the generated line-table changes are
  expected.
- `recompile_object()` from PR #1258 was not applicable to this local tree
  because this branch does not currently contain that efun entry point.

## Merged In `09ec81cbf4775ca1972c514f35f075611cec25a7`

The following additional official issue fixes were selectively merged or
manually backported:

- PR #1238, remaining runtime scope: object load-count/depth accounting now
  happens before `valid_read`, preventing recursive `valid_read` load paths from
  bypassing the protection.
- PR #1238, remaining runtime scope: parser package tokenization now preserves
  UTF-8 multibyte bytes for word, `STR`, and `OBJ` matching.
- PR #1244, partially: a class body followed directly by a variable name now
  produces a targeted diagnostic instead of a confusing generic parse failure.
- PR #1244, partially: `safe_apply()` and function-pointer callback exception
  paths now unwind the value stack from the saved call base instead of popping
  arguments twice.
- PR #1244, partially: async file/database callbacks and DNS resolve callbacks
  preserve `this_player()` when `this_player in call_out` compatibility is
  enabled.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- src/compiler/internal/grammar.autogen.cc src/compiler/internal/grammar.autogen.h src/compiler/internal/grammar.y src/packages/async/async.cc src/packages/core/dns.cc src/packages/parser/parser.cc src/vm/internal/apply.cc src/vm/internal/base/function.cc src/vm/internal/simulate.cc testsuite/clone/class788_ok.c testsuite/clone/class788_repro.c testsuite/single/tests/compiler/class_combined_decl.c testsuite/single/tests/crasher/1014.c testsuite/single/tests/efuns/async_this_player.c testsuite/single/tests/efuns/parse_utf8.c`

Notes:

- The Windows build regenerated `src/compiler/internal/grammar.autogen.cc` and
  `src/compiler/internal/grammar.autogen.h` from
  `src/compiler/internal/grammar.y`; the generated parser table changes are
  expected.
- The new async `this_player()` regression is intentionally side-effect-free.
  The local testsuite compiles and schedules those callback paths, while the
  broader async callback execution coverage still comes from the existing async
  tests.

## Merged In `9ddb8d623bfe972f64d7681400ce498b01199932`

The following official PR #1250 runtime/compiler behavior was selectively
merged or manually backported:

- Added public `to_buffer()` and internal `_to_buffer()` conversion support for
  strings, buffers, and integer arrays.
- Allowed buffer concatenation and compound assignment with buffers, strings,
  and integer arrays while preserving strict byte conversion.
- Allowed buffer range assignment from buffers, strings, and integer arrays.
- Enforced strict buffer byte writes and byte lvalue arithmetic in the
  `0..255` range instead of silently truncating.
- Added buffer `foreach` support, including writable byte refs for
  `foreach(int ref c in buffer_value)`.
- Reduced use of the global byte lvalue state for byte refs by carrying the
  byte pointer in the active lvalue/ref.
- Kept string `foreach ref` behavior conservative in this branch: the existing
  tests still assert that string refs do not mutate the source string.
- Added focused operator tests for buffer byte bounds, buffer range assignment,
  buffer foreach, and buffer refs.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/operators/buffer_bytes.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/operators/buffer_range_assign.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/operators/foreach.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/operators/ref.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check`

Notes:

- Official PR #1250 docs/sidebar changes were not imported because this branch
  uses local Chinese repository documentation and a different docs boundary.
- The official split compiler frontend files are not present in this branch;
  the behavior was adapted into the older local `grammar.y` / `lex.cc` layout.
- The Windows build regenerated `src/compiler/internal/grammar.autogen.cc` from
  `src/compiler/internal/grammar.y`; the generated parser table changes are
  expected.

## Merged In `726e990d17b11614ac9387c4b60cdc7f77bf9d73`

The following official PR #1247 fixes were selectively merged or manually
backported as a first partial safety batch:

- `write_buffer()` now rejects negative lengths and checks offset/length bounds
  without signed overflow.
- `read_file()` now clamps the terminator write to the actual bytes read after
  the forward line scan and maximum-size clamp.
- Reverse EGC search no longer loops forever when an unaligned match is found at
  offset zero.
- `sys_reload_tls()` now validates the requested port against the number of
  `external_port` entries instead of comparing an index to the byte size of the
  array.
- `lpcaddr_to_sockaddr()` now rejects host names that would overflow its fixed
  host buffer.
- `socket_accept()` now applies close-on-exec handling to the accepted OS socket
  fd and closes the accepted fd on that error path.
- MySQL binary/string fields now allocate buffers from the current row's actual
  BLOB length instead of the column-wide maximum length.
- Added focused regressions for `write_buffer()` bounds, reverse `strsrch()`,
  and invalid `sys_reload_tls()` port indexes.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/write_buffer_bounds.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/strsrch.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/sys_reload_tls.c`
- `..\build\dist\driver.exe etc\config.test -ftest *> ..\build\lpc-full-test-pr1247-partial.log`
- `git diff --check`

Notes:

- This is a partial merge of PR #1247, not a full PR merge.
- The full testsuite command exited with status 0 and wrote its log to
  `build/lpc-full-test-pr1247-partial.log`; that build artifact is not tracked.
- This batch intentionally focused on low-risk runtime safety fixes that do not
  require adopting official-only compiler layout, hot-reload, or larger package
  restructuring.

## Not Merged From The Reviewed Snapshot

These official changes remain intentionally unmerged as of `726e990d`:

- PR #1259: official `lpc-syntax` VS Code formatter wiring, tokenizer fixes,
  highlighter fixes, generated grammar-contract updates, and extension tests.
- PR #1258: the `recompile_object()` dangling filename-pointer fix is not
  applicable until this branch carries `recompile_object()`; any remaining
  Coverity items tied to official-only file layout also remain unmerged.
- PR #1257 and PRs #1253-#1255: official CI/release workflow restructuring and
  automatic release triggers.
- PR #1250, remaining scope: official docs/sidebar updates and any
  official-only string/ref test cases tied to the newer split compiler/test
  layout.
- PR #1247, remaining scope after the first partial safety batch: parser
  mid-parse destruct/use-after-free handling; allocator-initializer leak paths;
  additional `sprintf` fixes; `pcre_replace()` heap overflow work; async
  in-flight set/getdir path handling; `call_other()` type-check bounds;
  macro-parameter hang fixes; `random_number(0)` handling; string allocation
  hash-overflow protection; `uncompress()` and `terminal_colour()` leak fixes;
  `INT_MIN` division/modulo cases; `input_to` `#` apply handling; external
  socket teardown; matrix, mudlib stats, `get_dir()`, `add_action()`, and
  `call_out()` bounds; MySQL regression coverage; telnet LINEMODE/ZMP handling;
  and all `recompile_object()`, FFI, or newer compiler-layout-specific parts.
- PR #1245: char-mode input delivery improvements and NAWS-at-logon fix.
- PR #1244: remaining issue fixes not covered by this merge, including CRLF
  string-semantics test updates and any official-only cases tied to file layout
  or test harness differences.
- PR #1237: `recompile_object()` hot-reload efun and related master/simul_efun
  handling.
- PR #1231 and PR #1243: WebAssembly driver target and WASM size reductions.
- PR #1230: `inherit_program` and `include_file` master applies plus the auto
  hot-reload demo.
- PR #1210: the large LPC platform modernization batch, including Flex-based
  front-end work, clang-style diagnostics, arena compiles, `.lpc` source
  preference, FFI package, decimal library, `tools/lpc-syntax`, official VS Code
  extension, and generated syntax assets.
- Documentation-only updates such as Docusaurus/i18n/sidebar/search work, large
  efun doc expansions, and docs audit cleanup.

## Update Rules

When merging official functionality in the future:

1. Re-query `fluffos/fluffos` read-only and update the snapshot SHA/date above.
2. Add the local commit hash and a concise list of merged upstream changes.
3. Move any newly merged item out of the "Not Merged" section or mark it
   partially merged with the remaining scope.
4. Record validation commands and any skipped coverage.
5. Keep the official repository read-only: no remote, branch push, PR edit, or
   release action against `fluffos/fluffos`.
