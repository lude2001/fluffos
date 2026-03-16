# HTTP Protocol Helpers for LPC HTTP Services

## Summary

This design proposes a small set of HTTP-related driver helpers to remove protocol glue code from LPC HTTP services without turning the driver into a full web framework.

The current LPC example in `lpc_example_http/httpd.c` mixes transport concerns, HTTP parsing, form decoding, routing, controller dispatch, IP filtering, and response formatting in one file. The heaviest part is not controller dispatch itself, but the protocol work around partial reads, request assembly, header parsing, URL decoding, and response construction.

The proposed change keeps routing and business logic in LPC while moving low-level, stable, protocol-oriented utilities into the driver.

## Problem

The current LPC implementation has several maintenance and correctness risks:

- A single socket read is treated as a complete HTTP request.
- Request headers and body are split with one `sscanf`, which assumes the full payload is already available.
- `Content-Length` is not used to assemble partial bodies across reads.
- Query string and form parsing only keep values and discard keys.
- URL decoding is ad hoc and duplicated in LPC code.
- Response strings are hand-built with `sprintf`.
- Header normalization is left to application code.

These are all protocol glue concerns. They are possible to implement in LPC, but they are tedious, repetitive, easy to get subtly wrong, and distract from actual application logic.

## Goals

- Keep HTTP routing and controller dispatch in LPC.
- Move repetitive HTTP protocol helpers into the driver.
- Provide a minimal API surface that is broadly useful and easy to test.
- Support incremental parsing from raw socket chunks.
- Decode URL query strings and URL-encoded form bodies into mappings.
- Provide a standard response builder to eliminate hand-written response formatting.

## Non-Goals

- Do not add a complete HTTP server framework to the driver.
- Do not move routing or controller lookup into the driver.
- Do not automatically invoke LPC callbacks from inside the parser.
- Do not implement multipart form parsing in this iteration.
- Do not add automatic JSON response generation in the driver.
- Do not tightly couple the parser to socket `fd` state in the public API.

## Current LPC Boundary

The intended LPC responsibilities remain:

- Route request path to controller and action.
- Validate allowed HTTP method.
- Apply IP whitelist checks.
- Invoke controller functions.
- Encode business data to JSON.
- Decide status codes and headers for application responses.

The intended driver responsibilities become:

- URL encoding and decoding.
- Query string and form decoding.
- HTTP request boundary detection.
- Header normalization.
- `Content-Length`-driven body assembly.
- Response string construction.

## Approaches Considered

### 1. Socket `fd`-bound parser

The driver stores parser state per socket descriptor and LPC feeds chunks by `fd`.

Pros:

- Minimal LPC state management.
- Very small call surface at the LPC layer.

Cons:

- Tightly couples HTTP parsing to socket internals.
- Harder to test independently from the socket layer.
- Less reusable for non-socket or offline parsing scenarios.

### 2. Parser handle plus helper efuns

LPC creates a parser handle per connection and feeds raw chunks into it. The driver owns parsing state, request assembly, header normalization, and form/query decoding. LPC receives completed request mappings.

Pros:

- Clean separation between transport and protocol helpers.
- Easy to test incrementally.
- Keeps routing and business logic in LPC.
- More reusable than an `fd`-bound design.

Cons:

- LPC must manage parser lifecycle per connection.
- Slightly larger public API than a fully `fd`-bound design.

Recommendation:

This is the recommended approach. It moves the fragile protocol work into the driver without introducing a heavy framework or over-coupling to the socket subsystem.

### 3. High-level driver-managed HTTP service

The driver would provide a higher-level service abstraction that accepts requests and dispatches controllers directly.

Pros:

- Smallest LPC implementation.

Cons:

- Pushes application design decisions into the driver.
- Harder to preserve existing LPC controller patterns.
- Too opinionated for the current goal.

This approach is intentionally rejected.

## Proposed API

### String and form helpers

```c
string url_decode(string s);
string url_encode(string s);
mapping http_decode_query(string s);
mapping http_decode_form(string body);
```

Behavior:

- `url_decode` decodes percent-encoded sequences and treats `+` as space where appropriate for query/form data.
- `url_encode` encodes reserved characters into a safe URL form.
- `http_decode_query` converts a query string such as `a=1&b=2` into a mapping.
- `http_decode_form` decodes `application/x-www-form-urlencoded` payloads into a mapping.

Initial mapping behavior:

- Keys and values are decoded strings.
- Empty values are allowed.
- Repeated keys may either collect into an array or use last-write-wins semantics. For this iteration, last-write-wins is the simpler default unless the existing codebase strongly prefers arrays.

### Stateful HTTP parser helpers

```c
mixed http_parser_create();
mapping http_parser_feed(mixed parser, string chunk);
void http_parser_close(mixed parser);
```

Behavior:

- `http_parser_create` returns an opaque parser handle.
- `http_parser_feed` accepts the next raw chunk and returns a structured result.
- `http_parser_close` frees parser state.

`http_parser_feed` returns a mapping with this shape:

```c
([
  "requests": ({ /* zero or more request mappings */ }),
  "error": 0,
  "need_more": 1,
])
```

Or, on parse failure:

```c
([
  "requests": ({}),
  "error": ([
    "status": 400,
    "reason": "malformed_header",
    "message": "Invalid HTTP header line",
  ]),
  "need_more": 0,
])
```

Semantics:

- `requests` contains zero or more complete requests assembled from the current parser buffer.
- `need_more` is true when no error occurred but the parser needs more bytes to finish the current request.
- `error` is either `0` or a structured error mapping suitable for direct HTTP error handling in LPC.

The array form for `requests` allows one chunk to yield zero, one, or multiple complete requests.

### HTTP response builder

```c
string http_build_response(int status, mapping headers, string body);
```

Behavior:

- Builds a valid HTTP response string.
- Uses a built-in reason phrase for common status codes.
- Auto-populates `Content-Length`.
- Defaults `Connection` to `close` when not provided.
- Leaves body generation to LPC.

This helper does not encode JSON and does not choose content type automatically.

## Request Mapping Shape

Each complete request returned by `http_parser_feed` should use a stable, plain mapping shape:

```c
([
  "method": "POST",
  "path": "/update_code/update_file",
  "version": "1.1",
  "query_string": "file_name=%2Fadm%2Fdaemon%2Ffoo.c",
  "query": ([ "file_name": "/adm/daemon/foo.c" ]),
  "headers": ([
    "content-type": "application/x-www-form-urlencoded",
    "content-length": "31",
  ]),
  "body": "file_name=%2Fadm%2Fdaemon%2Ffoo.c",
  "form": ([ "file_name": "/adm/daemon/foo.c" ]),
])
```

Notes:

- Header keys are normalized to lowercase.
- Both raw and decoded request data are preserved.
- `form` is populated for `application/x-www-form-urlencoded` only.
- Request metadata such as remote address remains outside the parser for now and can still be obtained from socket APIs in LPC.

## LPC Integration Plan

`lpc_example_http/httpd.c` becomes much thinner:

1. Accept a socket connection.
2. Create a parser handle for that connection.
3. Feed each raw chunk to `http_parser_feed`.
4. If `need_more`, wait for more data.
5. If `error`, build an error response with `http_build_response` and close.
6. For each request:
   - parse controller and function from the path
   - validate method
   - check IP whitelist
   - call the controller
   - `json_encode` the result
   - use `http_build_response` to format the response
7. Close the parser when the connection ends.

This removes the need for LPC to implement:

- request-line and header/body splitting
- partial body assembly
- header normalization
- URL decoding helpers scattered across controllers
- handwritten response formatting

## Compatibility Strategy

The initial migration should preserve current controller behavior as much as possible.

Short term:

- Controllers can continue to receive positional string arguments if the HTTP daemon maps decoded query/form values into the existing call shape.
- The HTTP daemon becomes the only place that translates from request mappings to current controller invocation style.

Later, if desired:

- Controllers may be upgraded to accept a full request mapping directly.
- That change should be optional and separate from the protocol-helper introduction.

This staged approach keeps the protocol cleanup independent from business-layer interface changes.

## Error Handling

The parser should report structured HTTP-friendly errors instead of throwing generic LPC-visible failures for malformed input.

Expected categories include:

- malformed request line
- malformed header line
- missing or invalid `Content-Length`
- incomplete body
- unsupported transfer mode

This lets LPC return a standard error response without guessing how to interpret parser failures.

## Testing Strategy

The highest-value tests are protocol boundary tests rather than routing tests.

Required coverage:

- request split across multiple reads
- request headers in one read and body in later reads
- complete request in a single read
- multiple complete requests in one read
- valid and invalid `Content-Length`
- URL decoding for `%xx` and `+`
- query string decoding into mappings
- form decoding for `application/x-www-form-urlencoded`
- header normalization to lowercase keys
- response `Content-Length` correctness for UTF-8 content

Recommended testing layers:

- driver-level unit tests for parser behavior and helper efuns
- LPC example tests to verify the simplified daemon flow still works end to end

## Open Choices

These are intentionally deferred unless implementation pressure says otherwise:

- whether repeated query/form keys should collect into arrays
- whether `HEAD` should suppress body emission in `http_build_response` or remain an LPC concern
- whether a future parser version should expose additional metadata such as raw request line or connection flags

## Recommendation

Implement the parser-handle design with these efuns:

- `url_decode`
- `url_encode`
- `http_decode_query`
- `http_decode_form`
- `http_parser_create`
- `http_parser_feed`
- `http_parser_close`
- `http_build_response`

This is the smallest useful set that meaningfully reduces LPC protocol glue while preserving application control in LPC.
