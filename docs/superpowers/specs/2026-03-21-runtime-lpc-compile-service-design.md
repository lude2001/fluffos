# Runtime LPC Compile Service Design

**Goal:** Expose FluffOS's real LPC compile-and-load behavior to local development tools through a driver-owned IPC service, so external programs can request file or directory recompilation and receive structured diagnostics without booting a separate temporary environment.

**Context:**
- LPC compilation is defined by the running FluffOS VM and its current mud environment, not by a standalone compiler process.
- A valid compile request depends on the live runtime context established by the active config file, including mudlib root, include paths, master object, simul_efun object, and package configuration.
- Starting a separate temporary process per request would be too slow, too wasteful, and would not reflect the actual running driver state that developers care about.

**Core decision:**
- The running `driver` process owns the compile service.
- A new local CLI client, `lpccp`, sends compile requests to that running driver over local IPC.
- Requests use a stable `id` derived from the config file identity of the running driver instance.
- Compilation uses real object reload semantics inside the current VM rather than a separate pure-analysis path.

**Non-goals:**
- No TCP or HTTP transport in the first version.
- No mudlib socket or mudlib HTTP forwarding layer.
- No temporary one-shot `lpcc`-style boot per request.
- No dependency-graph planner or parallel compilation scheduler.

**Identifier model:**
- Each running driver instance is identified by its normalized config file path.
- The compile service derives a stable `id` from that config path.
- Repository assumptions guarantee that one config file is not used by two concurrent driver processes, so config identity is sufficient.
- `lpccp` never discovers processes dynamically; it resolves a local IPC endpoint from the provided `id`.

**Transport:**
- Use local IPC only.
- On Windows, the first implementation uses a named pipe:
  - `\\\\.\\pipe\\fluffos-lpccp-<id>`
- The driver creates the pipe during startup after config loading is complete.
- The client connects, sends one request, receives one response, and disconnects.

**Request model:**
- `lpccp <id> <path>`
- `<path>` may be:
  - A single LPC source file
  - A directory to compile recursively
- Internally, the request payload is JSON:

```json
{"version":1,"op":"compile","target":"/adm/foo/bar.c"}
```

or

```json
{"version":1,"op":"compile","target":"/adm/foo/"}
```

**Response model:**
- The service always returns JSON.
- File compile response:

```json
{
  "version": 1,
  "ok": false,
  "kind": "file",
  "target": "/adm/foo/bar.c",
  "diagnostics": [
    {
      "severity": "warning",
      "file": "/adm/foo/bar.c",
      "line": 12,
      "message": "Unused local variable 'x'"
    }
  ]
}
```

- Directory compile response:

```json
{
  "version": 1,
  "ok": false,
  "kind": "directory",
  "target": "/adm/foo/",
  "files_total": 12,
  "files_ok": 10,
  "files_failed": 2,
  "results": [
    {
      "file": "/adm/foo/a.c",
      "ok": true,
      "diagnostics": []
    },
    {
      "file": "/adm/foo/b.c",
      "ok": false,
      "diagnostics": [
        {
          "severity": "warning",
          "file": "/adm/foo/b.c",
          "line": 12,
          "message": "Unused local variable 'x'"
        }
      ]
    }
  ]
}
```

**Compilation semantics:**
- This service performs real runtime recompilation inside the active driver.
- The service does not use `reload_object()` because `reload_object()` resets runtime state but does not reparse source or load new code.
- For each target file:
  - Normalize the target to the object path used by the driver.
  - If the object is already loaded, find it with `find_object2()`.
  - Destruct the loaded object with `destruct_object()`.
  - Flush pending destruct cleanup with `remove_destructed_objects()`.
  - Recompile and reload by calling the normal object loading path.
- Successful compilation means the running driver now holds the newly compiled object.
- Failed compilation means the object may remain absent after the request, which is acceptable for development-mode behavior.

**Directory compilation semantics:**
- Recursively scan the target directory for `.c` files.
- Sort files lexicographically before compilation so execution order is stable and predictable.
- Compile files one by one using the same single-file runtime recompilation path.
- Do not stop on first failure.
- Aggregate all per-file results into one JSON response.

**Diagnostics collection:**
- Reuse the driver's existing compiler diagnostics pipeline rather than inventing a separate warning system.
- `smart_log()` remains the canonical source for compile warnings and errors.
- During an IPC compile request, the driver attaches a request-scoped diagnostics collector.
- `smart_log()` continues to:
  - emit the normal debug output
  - invoke `master->log_error`
- It also appends a structured diagnostic record into the active request collector when one exists.
- This preserves current runtime behavior while making the same information consumable by tools.

**Concurrency model:**
- Compile requests are processed serially.
- The current FluffOS compile path is not reentrant, so parallel compilation requests are explicitly out of scope.
- If the service receives a new request while one is active, it should either queue it or reject it with a busy error. The first version should prefer a simple serialized queue.

**Failure handling:**
- IPC connection failure returns a client-side error from `lpccp`.
- Invalid `id` means no matching local pipe endpoint could be found.
- Invalid path, access denial, missing files, and parser errors are returned as normal diagnostics or fatal request errors depending on where they occur.
- Directory requests should preserve partial results even when one file fails.

**Implementation split:**
- `driver`
  - owns service startup and shutdown
  - resolves config-path-based `id`
  - listens on named pipe
  - executes compile requests
  - collects diagnostics
- `lpccp`
  - parses CLI arguments
  - resolves named pipe from `id`
  - sends one compile request
  - prints response JSON
  - sets process exit code from response outcome

**Expected source changes:**
- Add a compile service module for IPC server behavior.
- Add a request-scoped diagnostics collector for structured compiler messages.
- Hook service startup into driver initialization and shutdown.
- Add a new `lpccp` executable entry point.
- Add a runtime compile helper that performs force-destruct plus real reload for one LPC file.
- Add directory traversal and aggregation logic.

**Testing strategy:**
- Unit-level validation for diagnostics collector formatting.
- Integration test for single-file compile through the service.
- Integration test for directory compile aggregation.
- Regression test proving changed source is actually recompiled, not merely reset.
- Failure test for syntax errors returning structured diagnostics.

**Trade-offs:**
- Using real runtime recompilation keeps semantics aligned with actual LPC behavior, but it does allow development-time compile requests to affect live object state.
- This is acceptable because the feature is explicitly intended for local development environments.
- Avoiding a separate pure-analysis path keeps the implementation smaller and the results closer to what developers see in practice.

**Open assumptions carried into implementation:**
- Compile service is local-development-only.
- One config file maps to one running driver instance.
- Windows named pipes are the initial target transport.
- Existing warning behavior controlled by LPC pragmas remains unchanged; the service reports what the driver actually emits.
