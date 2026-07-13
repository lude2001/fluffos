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
- Latest local merge commit: `bab352bad1679d7f838e72cd9c121faf8008a483`
  (`test: cover optional mysql binary row lengths`)
- Previous local merge commit:
  `ae8a687fa34a442c0891c6109ba342e734e84277`
  (`test: cover buffer range assignment backport`)
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

## Merged In `9414cfe9bb775ef10b2c3bfe3188c09aeebf7f86`

The following official PR #1247 fixes were selectively merged or manually
backported as a second partial safety/bounds batch:

- `random_number()` and `secure_random_number()` now return `0` for non-positive
  bounds before constructing a random distribution.
- Shared-string hash table sizing now caps the power-of-two growth loop before
  signed integer overflow.
- `pcre_replace()` now uses the same non-overlapping capture-group selection in
  its sizing and copy passes, avoiding heap overwrite on nested captures.
- `uncompress()` now releases partial output data on inflate failure.
- `terminal_colour()` now uses `safe_apply()` for
  `terminal_colour_replace()` so callback errors do not leak local buffers.
- `repeat_string()` now clamps before multiplying string length by repeat count.
- `replace_string()` now bounds and accounts for the Boyer-Moore skip-copy fast
  path.
- `get_dir()` now bounds directory/file path concatenation before `stat()`.
- `add_action()` missing-function error construction now uses bounded formatting
  and does not pass user-controlled text as the format string.
- `call_out()` now handles null function-pointer owners during reclaim and
  saturates seconds-to-milliseconds conversion.
- Matrix transforms now reject arrays shorter than the required 16 elements.
- Mudlib stats author/domain and stat-file restore paths now bound fixed-buffer
  copies/scans.
- Macro parameter parsing now stops on malformed parameter names instead of
  continuing with an unconsumed character.
- Constant-folded `INT_MIN / -1` and `% -1` cases now avoid C++ undefined
  behavior in `#if`, expression division, and integer modulo folding.
- Added focused regressions for pcre replacement, random/secure_random bounds,
  matrix short arrays, large call_out delays, macro parameter errors,
  terminal_colour callback errors, uncompress failure cleanup, and 64-bit
  integer division/modulo folding.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/pcre.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/random.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/secure_random.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/matrix.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/call_out.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/64bit.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/bad_macro_params.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/crasher/terminal_colour_error_replace.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/crasher/uncompress_invalid.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/replace_string.c`
- `..\build\dist\driver.exe etc\config.test -ftest *> ..\build\lpc-full-test-pr1247-second-batch.log`
- `git diff --check`

Notes:

- This is still a partial merge of PR #1247, not a full PR merge.
- The full testsuite command exited with status 0 and wrote its log to
  `build/lpc-full-test-pr1247-second-batch.log`; that build artifact is not
  tracked.
- The Windows build regenerated `src/compiler/internal/grammar.autogen.cc` from
  `src/compiler/internal/grammar.y`; the generated parser table changes are
  expected.

## Merged In `ed01cbc055924f13df67cd4bd62795db2a96defb`

The following official PR #1247 async fixes were selectively merged or manually
backported as a third partial batch:

- Async worker requests that have been popped from the queue but not yet moved
  to the finished queue are now tracked in `current_works` so debugmalloc
  marking accounts for their callback funptr and captured `command_giver`.
- `async_getdir()` now grows its raw directory-entry buffer by
  `sizeof(struct dirent)` per entry instead of `sizeof(dirent *)`.
- `async_read()`, `async_getdir()`, and `async_write()` now release the callback
  function object on permission-denied paths where no request will be queued.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/async_this_player.c`
- `..\build\dist\driver.exe etc\config.test -ftest *> ..\build\lpc-full-test-pr1247-async.log`
- `git diff --check`

Notes:

- This is still a partial merge of PR #1247, not a full PR merge.
- The full testsuite command exited with status 0 and wrote its log to
  `build/lpc-full-test-pr1247-async.log`; that build artifact is not tracked.
- A standalone local run of `/single/tests/efuns/async.c` printed
  `Checks succeeded` but did not exit before the command timeout in this
  harness, so it was not used as a clean gating signal for this commit.

## Merged In `e4eda115aa6d42adc384920f49be49b435a51d9c`

The following official PR #1247 interactive-input fix was selectively merged or
manually backported as a fourth partial batch:

- `input_to()` now rejects string callbacks whose function name starts with the
  internal apply marker `#` before preparing the VM call frame. The rejection
  path releases the pending input sentence, referenced object, and carryover
  arguments instead of returning from the later stack-setup path.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/input_to.c`
- `..\build\dist\driver.exe etc\config.test -ftest *> ..\build\lpc-full-test-pr1247-input-to.log`
- `git diff --check -- src/comm.cc`

Notes:

- This is still a partial merge of PR #1247, not a full PR merge.
- The full testsuite command exited with status 0 and wrote its log to
  `build/lpc-full-test-pr1247-input-to.log`; that build artifact is not
  tracked.
- The focused `input_to` test covers the existing non-interactive setup path.
  The `#` apply branch is an interactive safety path, so this merge is gated by
  the Windows build, the existing `input_to` test, and the full LPC testsuite
  rather than a dedicated live socket regression.

## Merged In `b6a529611506f9350877d859d1039f6edb424732`

The following official PR #1247 external-process socket cleanup was selectively
merged or manually backported as a fifth partial batch:

- `external_start()` now closes the provisioned efun socket with
  `socket_close(fd, SC_FORCE | SC_FINAL_CLOSE)` when `posix_spawn()` fails after
  the socket has been registered, then clears `sv[0]` so the deferred raw-fd
  cleanup does not double-close it.
- The `socket_close()` internal flags are exposed through `socket_efuns.h` so
  the external package can use the same forced-final close path as the socket
  package.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/sockets.c`
- `..\build\dist\driver.exe etc\config.test -ftest *> ..\build\lpc-full-test-pr1247-external.log`
- `git diff --check -- src/packages/external/external.cc src/packages/sockets/socket_efuns.cc src/packages/sockets/socket_efuns.h`

Notes:

- This is still a partial merge of PR #1247, not a full PR merge.
- The full testsuite command exited with status 0 and wrote its log to
  `build/lpc-full-test-pr1247-external.log`; that build artifact is not
  tracked.
- The affected `posix_spawn()` failure path is in the non-Windows external
  implementation. The local Windows validation proves the shared socket API
  exposure and existing socket behavior still build and pass, but it is not a
  dedicated POSIX runtime reproduction of the spawn-failure cleanup path.

## Merged In `94c3029bee39d9528c9243debc1757c85c8f429f`

The following official PR #1247 `call_other()` type-checking fix was selectively
merged or manually backported as a sixth partial batch:

- `check_co_args()` now passes the bounded declared-argument count to
  `check_co_args2()` when reading `prog->argument_types`, while keeping the full
  pushed argument count for stack indexing. Extra actual arguments no longer
  make the type checker read beyond the declared argument type table.
- `call_other()` type-check error strings are now emitted with `error("%s",
  buf)` instead of treating object-derived text as a printf format string.
- The existing `call_other` efun regression now temporarily enables runtime
  call-other type checking and exercises the extra-argument error path.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/call_other.c`
- `..\build\dist\driver.exe etc\config.test -ftest *> ..\build\lpc-full-test-pr1247-call-other.log`
- `git diff --check -- src/vm/internal/apply.cc testsuite/single/tests/efuns/call_other.c`

Notes:

- This is still a partial merge of PR #1247, not a full PR merge.
- The full testsuite command exited with status 0 and wrote its log to
  `build/lpc-full-test-pr1247-call-other.log`; that build artifact is not
  tracked.

## Merged In `797ca93624c7543a54ba3e453d0f96af25be5f2a`

The following official PR #1247 parser lifetime fix was selectively merged or
manually backported as a seventh partial batch:

- Parser verb nodes removed during an active parse are now deferred until the
  outermost parse unwinds, preventing `parse_vn` from pointing at freed memory
  if a handler destructs itself or calls `parse_remove()` from a
  `can_`/`direct_`/`do_` apply.
- `clear_result()` initializes each saved argument count to zero, so
  error-path cleanup never reads uninitialized counts.
- If a handler is already destructed when `we_are_finished()` commits a
  candidate, the half-built result is freed and `best_match` is cleared so the
  parser does not call `do_the_call()` with a null/destructed target.
- Added `/single/tests/crasher/parser_handler_destruct.c` to cover a handler
  destructing itself from `direct_*` while returning success.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/crasher/parser_handler_destruct.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/parse_utf8.c`
- `..\build\dist\driver.exe etc\config.test -ftest *> ..\build\lpc-full-test-pr1247-parser-uaf.log`
- `git diff --check -- src/packages/parser/parser.cc testsuite/single/tests/crasher/parser_handler_destruct.c`

Notes:

- This is still a partial merge of PR #1247, not a full PR merge.
- The full testsuite command exited with status 0 and wrote its log to
  `build/lpc-full-test-pr1247-parser-uaf.log`; that build artifact is not
  tracked.
- An initial parallel run of the focused parser tests left a local
  `build/dist/driver.exe` test process after timeout; it was stopped by exact
  path before rerunning `parse_utf8.c` successfully. The separate production
  `D:\code_env\FluffOS\libexec\fluffos\driver.exe` process was not touched.

## Merged In `a8fada2406154b2ab0942b85d00388114d29d730`

The following official PR #1247 fixes were selectively merged or manually
backported as an eighth partial runtime-hardening batch:

- `allocate(n, function)` now keeps the partially built result array on the VM
  stack while running per-element callbacks, so callback errors do not leak the
  result or already-stored refcounted values.
- Telnet LINEMODE subnegotiation now checks the second suboption byte exists
  before reading or echoing it, and ZMP argument arrays fill `item[0..n-1]`
  instead of writing one slot past the end.
- `query_replaced_program()` without an explicit object now reads
  `current_object->replaced_program` instead of treating the arbitrary stack top
  as an object.
- `replaceable(ob, ({}))` now allocates the built-in ignore entries even for an
  empty caller ignore list.
- `compose_mapping()` now frees deleted mapping keys before freeing the node.
- `sprintf()` now prints integer/float values with bounded `snprintf()` for
  huge user-supplied precision.
- The disassembler now treats the direct switch-table `minval` as
  `sizeof(LPC_INT)` instead of a hardcoded four bytes.
- Compiler overload warnings and trace lines now pass source-derived text as a
  `"%s"` argument instead of a printf format string.
- `strftime()` now uses heap storage rather than a stack VLA sized by
  `__MAX_STRING_LENGTH__`.
- `checkmemory` now bounds the configured default fail-message copy.
- `norm()` now uses the array passed to the helper when measuring length, fixing
  the shared helper path used by `angle()`.
- MUD-port input now clamps read length to the local buffer and rejects
  non-positive length prefixes.
- Added `/single/tests/crasher/replaceable_empty.c` and extended the existing
  `allocate` and `sprintf` efun regressions.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/crasher/replaceable_empty.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/allocate.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/sprintf.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/replaceable.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check`

Notes:

- This is still a partial merge of PR #1247, not a full PR merge.
- The full testsuite command exited with status 0. Its output was observed in
  the terminal and was not redirected to a tracked artifact.

## Merged In `1b03c79fa98733e8b7265f56eb4d27bb5003b017`

The following official PR #1239/#1241 preprocessor directive comment behavior
was selectively merged or manually backported into this branch's older
`src/compiler/internal/lex.cc` layout:

- Block comments inside preprocessor directive payloads now fold to whitespace
  instead of being removed entirely, so macro bodies such as
  `1 -/* comment */-1` keep token boundaries.
- Directive-line block comments that span physical lines remain consumed as
  part of the directive, preventing continuation text from being tokenized as
  normal LPC code.
- Existing local trailing-line-comment handling for directive payloads is now
  pinned by tests for `#define`, `#ifdef`, `#undef`, and nested macro argument
  expansion.
- Added `/single/tests/compiler/preprocessor.c` to cover block-comment tails,
  live and dead `#if` branches, string literals containing comment markers,
  comment-as-whitespace token separation, trailing `//` macro comments, and
  `#ifdef`/`#undef` lookup with trailing comments.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/preprocessor.c`
- `git diff --check -- src/compiler/internal/lex.cc testsuite/single/tests/compiler/preprocessor.c`

Notes:

- This is a local-layout backport of PR #1239/#1241 behavior, not a wholesale
  import of official `lexer_rules_pp.cc`, generated Flex scanner changes, or
  official GTest compiler harness changes.
- The initial focused test failed before rebuilding the driver because the old
  dist binary still tokenized `1 -/* comment */-1` as `1--1`; after rebuilding,
  the focused test passed.

## Merged In `56c00880b40692bbc12a803026dd739e042859f2`

The following official PR #1230 compile-time master apply behavior was
selectively merged or manually backported into this branch's older compiler and
lexer layout:

- Added `inherit_program(string from, string path, int priv)` as a master apply
  consulted for each LPC `inherit` statement.
- `inherit_program()` can keep the default path, redirect to another program,
  supply inline source as an array of strings, or deny the inheritance.
- Added `include_file(string compiled, string from, string path)` as a master
  apply consulted for each `#include` directive.
- `include_file()` can keep the default path, redirect to another include file,
  supply inline include text as an array of strings, or deny the include.
- Added `StringLexStream` so master-supplied inline include/inherit text can be
  compiled through the same lexer stream abstraction as disk files.
- Added `load_object_from_source()` for synthesized inherited programs and made
  it support the normal inherited-parent retry flow, including inline source
  that itself inherits unloaded disk or synthesized parents.
- Added testsuite master hook forwarding through `set_compile_hooks()` for
  focused compiler tests.
- Added focused tests and fixtures for inherit redirects, inline inherited
  programs, denied inherits, private-inherit flags, include redirects, inline
  includes, nested include call shapes, and nested inline inherit source.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/inherit_program.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/include_file.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/get_include_path.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/preprocessor.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check`

Notes:

- This is a core-behavior merge of PR #1230, adapted to this branch's
  `grammar_rules.cc` / `lex.cc` compiler layout rather than importing official
  `grammar_rules.cc` / `lexer_utils.cc` changes wholesale.
- The official hot-reload daemon/demo from PR #1230 was not part of this
  commit. It was later adapted into this branch's `.c` testsuite layout in
  `362f6fefae58197c27baa7243ac494db6882c9dd`.
- The new master applies preserve existing production behavior when the mudlib
  master returns the original path or does not route to a custom hook.

## Merged In `99aa8be9e5db99f26d503bec7beae6ff32856921`

The following official PR #1247/#1258 compiler-hardening items were selectively
merged or manually reconciled with this branch's older `lex.cc` /
`compiler.cc` layout:

- The local old compiler had an additional precomposed warning buffer in
  `define_new_function()` that could include source-derived function/program
  names. It now calls `yywarn("%s", buff)` instead of treating that buffer as a
  printf format string.
- The local-name allocator's normal block path now copies the already-computed
  byte length with `memcpy()` rather than re-scanning with `strcpy()`, matching
  the safer official hardening direction around local-name storage.
- Added `/single/tests/compiler/long_local_name.c` as local coverage for a
  near-`MAXLINE` local/global-name compile path.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/long_local_name.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/crasher/replaceable_empty.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/include_file.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/compiler/preprocessor.c`
- `git diff --check -- src\compiler\internal\compiler.cc src\compiler\internal\lex.cc testsuite\single\tests\compiler\long_local_name.c`

Notes:

- This is not a full merge of PR #1247 or PR #1258.
- Official #1258's `recompile_object()` dangling filename-pointer fix remains
  pending until this branch carries `recompile_object()` from PR #1237.
- Official's `>4096` identifier regression cannot be expressed directly in this
  branch's LPC source tests because the old lexer enforces `MAXLINE=4096` first;
  the added test therefore pins the largest local source-level case this branch
  can compile without changing lexer line-length semantics.

## Merged In `fa228dce2b4ca9eb7f7219474dac2b4014d2d13d`

The following official PR #1244 string-semantics coverage was selectively
merged into this branch's existing `string_index.c` regression suite:

- Documented the intentional virtual-NUL result when indexing a string at
  `strlen(s)`.
- Added CRLF extended-grapheme-cluster coverage: `strlen("\r\n") == 1`,
  indexing `"\r\n"` returns the CR codepoint at cluster 0 and virtual NUL at
  cluster 1, and `strsrch()` only matches on cluster boundaries.

Validation for this merge:

- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/operators/string_index.c`
- `git diff --check -- src\base\internal\strutils.cc testsuite\single\tests\operators\string_index.c`

Notes:

- This is a test/comment merge of PR #1244 behavior. The runtime behavior was
  already present in this branch; the merge pins it so future string changes do
  not accidentally reinterpret CRLF as two searchable/indexable clusters.

## Merged In `06719fab6466eed700d3bc6474b23dd02c27adca`

The following official PR #1237 hot-reload foundation was selectively merged:

- Object global variables now live in a separate `TAG_OBJ_VARS` allocation
  instead of the tail of `object_t`. The block is always at least one `svalue_t`
  and is allocated through `allocate_object_variables()`.
- `object_t` now carries `prog_generation`, the field later used by
  `recompile_object()` to invalidate stale function pointers after a program
  swap.
- Debug-memory checking now marks object variable blocks from their owning
  object and reports orphan `TAG_OBJ_VARS` blocks explicitly.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- src/base/internal/debugmalloc.h src/vm/internal/base/object.h src/vm/internal/base/object.cc src/packages/develop/checkmemory.cc`

Notes:

- This is a partial merge of PR #1237, not the full `recompile_object()` efun.
- No public LPC API is exposed by this commit. Existing production mudlibs keep
  using the same object variables through `ob->variables`; the allocation shape
  changes only inside the driver.
- The remaining PR #1237 work includes the actual efun, live program swap,
  variable migration by name, master/simul_efun handling, function-pointer
  generation checks, executing-frame guards, clone/virtual/call_out/heart_beat
  safety handling, and focused regression tests.

## Merged In `3ebd18267178a324798779a4ae19be90306fe0dc`

The following official PR #1237 function-pointer foundation was selectively
merged:

- Function pointer headers now snapshot the owner's `prog_generation` at
  creation or `bind()` time.
- `call_function_pointer()` now rejects stale `FP_LOCAL` and `FP_FUNCTIONAL`
  pointers when their owner object has moved to a newer program generation.
- `FP_LOCAL` pointers now store the creation-time `program_t` and hold/release
  `func_ref` against that program, rather than decrementing the owner's current
  program after a later swap.
- Debug-memory checking now accounts `FP_LOCAL` extra function refs against the
  stored creation program.
- `%O` formatting now prints local function pointers from the stored creation
  program and tolerates removed simul_efun table entries.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- src/vm/internal/base/function.h src/vm/internal/base/function.cc src/packages/core/efuns_main.cc src/packages/develop/checkmemory.cc src/packages/core/sprintf.cc`

Notes:

- This is still a partial merge of PR #1237. It does not expose
  `recompile_object()` yet, but it makes the later program swap safe for
  existing function-pointer lifetime and stale-layout checks.
- Current production mudlibs do not need changes. Existing function pointers
  behave the same until an object program generation is actually bumped by the
  later hot-reload efun.

## Merged In `721fa0e65938b5b60fcb71fe6b8b82078ee5d552`

The following official PR #1237 `recompile_object()` core behavior was
selectively merged:

- Added public `recompile_object(object)` efun.
- Recompiling a master copy now swaps the new program into the live master copy
  and its loaded clones in place.
- Global variables migrate by name so existing state survives compatible source
  changes, while newly introduced variables keep their initializer values.
- Master applies and simul_efun dispatch tables are rebuilt before running the
  recompiled program's `__INIT`.
- Stale function-pointer protection from the earlier `prog_generation` merge is
  now active when a program is replaced.
- Pending `replace_program()` state is cancelled at the swap point.
- Unsafe targets are rejected, including clone targets, objects whose old
  program is currently executing on the VM stack, nested `recompile_object()`
  calls, and objects that already have pending `replace_program()` state before
  the recompile begins.
- Added focused local regression coverage for master-and-clone updates,
  variable migration by name, new variable initializers, stale local function
  pointers, and clone-target rejection.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/recompile_object.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check`

Notes:

- This is a local-layout merge of the core efun and swap machinery from PR
  #1237. It is not a wholesale import of every official hot-reload demo,
  documentation page, or broad official regression fixture.
- Existing production mudlibs do not need changes unless they deliberately call
  `recompile_object()` or depend on the new hot-reload behavior.

## Merged In `9e2aed146a581a951c7e79bb153c7d583761bca1`

The following official PR #1258 `recompile_object()` Coverity fix was
selectively merged after PR #1237 became applicable in this branch:

- `recompile_object()` now passes the stable `old_prog->filename` pointer to
  the compiler instead of passing a temporary local string buffer.
- The source file descriptor opened for recompilation is closed through `DEFER`,
  so compile-error paths do not leak it.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/recompile_object.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- src/vm/internal/simulate.cc`

## Merged In `1b8c5cc68c32380d80b4f19f81456c09f62239ce`

The following official PR #1237 regression coverage was selectively adapted to
this branch's `.c` testsuite layout:

- Expanded `recompile_object.c` coverage for vanished source files, virtual
  object backing-source recompilation, `replace_program()` state during a swap,
  self-destructing and erroring `__INIT`, live simul_efun table rebuild, and
  master executing-frame rejection.
- Added `recompile_object2.c` coverage for catch_tell routing, shadow chains,
  name-based call_outs running the new program, stale function-pointer call_outs
  failing cleanly, add_action sentences, and heart_beat registration.
- Added a testsuite master `compile_object()` fixture for the
  `/data/recompile_object/virt` virtual-object case.

Validation for this merge:

- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/recompile_object.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/recompile_object2.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --cached --check`

Notes:

- The official post-run idle-master recompile fixture is not represented in the
  local testsuite because this branch's single-test harness shuts down
  immediately after the focused test. The local coverage still verifies the
  master executing-frame guard, and the code path rebuilds master applies when
  the master object itself is swapped.
- The focused `recompile_object2.c` run validates the synchronous portions of
  that test. The local branch does not rely on the official post-run delayed
  call_out verifier because the local single-test runner shuts down
  immediately; it instead verifies that name/funptr call_out handles survive
  recompile and remain removable.

## Merged In `362f6fefae58197c27baa7243ac494db6882c9dd`

The following official PR #1230/#1237 hot-reload demo and coverage was
selectively adapted as a testsuite-only development example:

- Added `/single/hot_reload.c`, a daemon that registers as the testsuite
  master's compile hooks, records include/inherit dependency edges, snapshots
  source file size+mtime, and reloads stale watched programs.
- The state-keeping reload path uses `recompile_object()` so master copies and
  live clones receive the new program in place while retaining compatible
  variables by name.
- The opt-out reload path uses destruct+load so master copies restart from
  initializers and existing clones keep their old program until mudlib code
  chooses to migrate or destruct them.
- Objects with `hot_reload_state()` / `hot_reload_restore()` use the
  cooperative destruct+load path and explicitly choose which state survives.
- Added `/single/tests/applies/hot_reload.c` to cover dependency graph
  recording, deepest include-of-inherited-program changes, shared dependency
  stale-set collection, compile-failure self-healing, deepest-first ancestor
  refresh, currently-executing guard record preservation, state-keeping reloads,
  clone updates, cooperative restore, and opt-out semantics.
- Adjusted `recompile_object2.c` so call_out survival coverage is synchronous
  in this branch's test runner and leaves no generated test files behind.

Validation for this merge:

- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/recompile_object2.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/applies/hot_reload.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- testsuite/single/hot_reload.c testsuite/single/tests/applies/hot_reload.c testsuite/single/tests/efuns/recompile_object2.c`

Notes:

- This is a testsuite development example, not production-default behavior.
  It activates only when loaded and enabled by the testsuite.
- The official documentation-site page for hot reload was not imported
  wholesale; local project docs summarize the boundary here and in the README.

## Merged In `af6ccca5849049377135f204b70d775c301dcf72`

The following remaining low-risk official PR #1247 safety/correctness fixes
were selectively adapted:

- `ed` now clamps expanded printed lines before copying to the caller buffer,
  bounds default filename reuse, caps typed filename collection, and reserves
  enough room for escaped replacement/pattern text.
- `sprintf()` column/table formatting now owns the rendered `%O`/string buffer
  used by pending column/table state instead of borrowing the transient clean
  buffer. Pending column/table pad and owned storage are released on unwind.
- `restore_variable()` / restore mapping paths now report malformed or
  too-deep top-level mapping sizing failures as restore errors, and clear the
  transient restore scratch state before `error()` paths that would otherwise
  longjmp past normal cleanup.
- `replace_dollars()` now checks output growth against the replacement string
  length actually written, not the matched pattern length.
- Added `sprintf_column_object.c` coverage and expanded `restore_variable.c`
  coverage for deeply nested crafted input.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/restore_variable.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/sprintf_column_object.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/sprintf.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/ed.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- src/packages/core/ed.cc src/packages/dwlib/dwlib.cc src/vm/internal/base/object.cc src/packages/core/sprintf.cc testsuite/single/tests/efuns/restore_variable.c testsuite/single/tests/efuns/sprintf_column_object.c`

Notes:

- `src/packages/dwlib/dwlib.cc` is adapted in source, but the current Windows
  build configuration does not enable `PACKAGE_DWLIB`; validation for that
  exact optional package path is therefore code-review/build-layout coverage,
  not a live LPC package test in this run.

## Merged In `b7e294247ee027115fb9333701b2d3ff29a245b0`

The following official PR #1247 reclaim safety fixes were selectively adapted:

- `reclaim_objects()` now keeps the internal `nested` recursion counter
  balanced when `check_svalue()` exceeds `MAX_RECURSION` and returns early.
- Reclaiming a destructed owner from an `FP_LOCAL` function pointer no longer
  decrements the program's `func_ref` immediately. The function pointer remains
  alive and later `dealloc_funp()` releases the stored creation program exactly
  once.
- Added `/clone/reclaim_fp_helper.c` and
  `/single/tests/crasher/reclaim_funptr_owner.c` to cover a destructed owner
  whose function pointer is retained, scheduled in a call_out, and then visited
  by `reclaim_objects()` again.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/crasher/reclaim_funptr_owner.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- src/packages/core/reclaim.cc testsuite/clone/reclaim_fp_helper.c testsuite/single/tests/crasher/reclaim_funptr_owner.c`

## Merged In `ae8a687fa34a442c0891c6109ba342e734e84277`

The following official PR #1247 buffer range regression coverage was adapted:

- Expanded `/single/tests/operators/buffer_range_assign.c` to cover
  size-changing range assignment from a buffer RHS in both grow and shrink
  directions. This pins the previously merged runtime fix that copies from
  `buffer_t::item` rather than the `buffer_t` header during reallocation.

Validation for this merge:

- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/operators/buffer_range_assign.c`
- `..\build\dist\driver.exe etc\config.test -ftest`
- `git diff --check -- testsuite/single/tests/operators/buffer_range_assign.c`

## Merged In `bab352bad1679d7f838e72cd9c121faf8008a483`

The following official PR #1247 MySQL regression coverage was adapted:

- Extended `/single/tests/efuns/db.c` so the MySQL section can run even when
  SQLite support is not enabled in the current Windows build.
- Added optional coverage for per-row binary field lengths: when
  `FT_MYSQL_HOST`, `FT_MYSQL_DB`, and `FT_MYSQL_USER` are present in the OS
  environment, the test creates a temporary `VARBINARY` table and verifies that
  `db_fetch()` returns each payload as a buffer sized to that row's actual
  binary length.
- Added those short MySQL test environment names to
  `/testsuite/etc/config.test`'s `get_os_env()` allow-list. The names are kept
  short so the existing config-line limit is not crossed.
- The coverage is intentionally optional. A normal developer machine or CI
  build without MySQL credentials now skips this MySQL live check without
  failing the DB efun test.

Validation for this merge:

- `.\build.cmd`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/db.c`
- `..\build\dist\driver.exe etc\config.test -ftest:/single/tests/efuns/get_os_env.c`
- `git diff --check -- testsuite/etc/config.test testsuite/single/tests/efuns/db.c`

Notes:

- A full testsuite run reached `/single/tests/efuns/db.c`, skipped the optional
  MySQL live check because the environment variables were not configured, and
  then later failed in the existing `/single/tests/efuns/async.c` callback path
  with `async_read()` returning `-1`. That later async failure is not caused by
  this MySQL coverage change and was not used as the gating signal for this
  commit.

## Not Merged From The Reviewed Snapshot

These official changes remain intentionally unmerged as of `bab352ba`:

- PR #1259: official `lpc-syntax` VS Code formatter wiring, tokenizer fixes,
  highlighter fixes, generated grammar-contract updates, and extension tests.
- PR #1258, remaining scope: any remaining Coverity items tied to
  official-only file layout remain unmerged. The lexer local-name, restore
  overflow, and `recompile_object()` dangling filename-pointer fixes are merged
  in local form.
- PR #1257 and PRs #1253-#1255: official CI/release workflow restructuring and
  automatic release triggers.
- PR #1250, remaining scope: official docs/sidebar updates and any
  official-only string/ref test cases tied to the newer split compiler/test
  layout.
- PR #1247, remaining scope after the MySQL optional coverage batch: remaining
  official-only tests, optional-package live coverage not enabled by the
  current Windows build, and any FFI or newer compiler-layout-specific parts.
- PR #1245: char-mode input delivery improvements and NAWS-at-logon fix.
- PR #1244: remaining issue fixes not covered by this merge, including any
  official-only cases tied to file layout or test harness differences.
- PR #1237, remaining scope: official hot-reload documentation-site page, the
  official post-run idle-master recompile fixture, and any official-only test
  harness shape. The core efun, live master/clone program swap, variable
  migration, master/simul_efun rebuild, executing-frame guard, clone-target
  rejection, virtual object coverage, call_out/add_action/heart_beat/shadow
  coverage, `replace_program()` guard, self-destruct/erroring `__INIT` coverage,
  stale function-pointer protection, and testsuite hot-reload demo coverage are
  merged in local form.
- PR #1231 and PR #1243: WebAssembly driver target and WASM size reductions.
- PR #1230, remaining scope: official documentation-site pages and any
  official-only test/doc shape not represented in this branch's local compiler
  hook and hot-reload tests.
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
