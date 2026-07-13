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
- Latest local merge commit: `c168e3aa07dcfbd193485a51a7387ce055bf7412`
  (`merge additional upstream runtime fixes`)
- Previous local merge commit: `c20b15e4 merge selected upstream runtime fixes and efuns`
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

## Not Merged From The Reviewed Snapshot

These official changes remain intentionally unmerged as of `c168e3aa`:

- PR #1259: official `lpc-syntax` VS Code formatter wiring, tokenizer fixes,
  highlighter fixes, generated grammar-contract updates, and extension tests.
- PR #1258: the `recompile_object()` dangling filename-pointer fix is not
  applicable until this branch carries `recompile_object()`; any remaining
  Coverity items tied to official-only file layout also remain unmerged.
- PR #1257 and PRs #1253-#1255: official CI/release workflow restructuring and
  automatic release triggers.
- PR #1250: string `foreach`/`ref` fixes, buffer-as-byte-array behavior,
  `to_buffer()`, strict byte range checks, and broader ref tests.
- PR #1247: the large multi-round memory-safety and correctness audit batch.
- PR #1245: char-mode input delivery improvements and NAWS-at-logon fix.
- PR #1244: remaining issue fixes not covered by this merge, including class
  declaration diagnostics, safe_apply value-stack underflow, async
  `this_player()` preservation, and CRLF string-semantics test updates.
- PR #1238: remaining tier-1 fixes not covered by this merge, including
  load-count-before-`valid_read` and parser UTF-8 tokenization.
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
