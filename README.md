# cFlask

`cFlask` is a small, from-scratch HTTP server and web-framework experiment written in C. It is a learning project: the code walks through the pieces normally hidden behind a framework—TCP sockets, HTTP parsing, routing, response serialization, middleware, and simple file-backed HTML rendering.

It is deliberately compact and readable, making it a good codebase for following a request from the socket all the way to a handler.

> **Project status:** experimental / educational. The current server is useful for local exploration, but is not hardened for production use. See [Current limitations](#current-limitations) before exposing it to a network.

## What it can do

- Listen for HTTP/1.1 requests over a TCP socket.
- Parse request lines, headers, bodies, and `Content-Length`.
- Register `GET` and `POST` routes.
- Match static routes and named path parameters such as `/users/:id`.
- Parse simple query strings into a hashmap.
- Run middleware before a route handler, with the option to short-circuit a request.
- Return text, JSON, HTML, and 404 responses with `Content-Type` and `Content-Length` headers.
- Render an HTML file from `templates/`, with basic path-traversal checks.
- Build with AddressSanitizer enabled to make memory errors easier to catch while learning.

## Requirements

- A Unix-like system with POSIX sockets (Linux/macOS/WSL are suitable).
- `gcc` and `make`.

The project currently uses GCC's `strdup` and POSIX networking APIs; it is not set up for Windows' native toolchain.

## Quick start

From the repository root:

```bash
make
./compiled/server
```

The example server listens on port `8080`. In another terminal, try its built-in routes:

```bash
curl -i http://localhost:8080/
curl -i 'http://localhost:8080/?category=c'
curl -i http://localhost:8080/users
curl -i http://localhost:8080/users/42
curl -i http://localhost:8080/users/42/posts/7
curl -i http://localhost:8080/does-not-exist
```

Expected highlights:

| Request | Result |
| --- | --- |
| `GET /` | `200 OK` with `home` |
| `GET /users` | `200 OK` and `templates/index.html` |
| `GET /users/42` | `200 OK` with `42` |
| `GET /users/42/posts/7` | `200 OK`; both parameters are available to the handler |
| Any unregistered route | `404 Not Found` |

Stop the foreground server with `Ctrl-C`. To remove the generated server executable, run:

```bash
make clean
```

## Your first route

The runnable application lives in [`src/server.c`](src/server.c). A handler receives a parsed `http_request_t` and returns a heap-allocated `http_response_t` created by one of the response helpers.

```c
#include "app.h"
#include "http.h"

static http_response_t* hello_handler(http_request_t* request) {
    (void)request;
    return http_text_response("Hello from cFlask!\n");
}

int main(void) {
    app_t* app = app_create();
    if (app == NULL) return 1;

    app_get(app, "/hello", hello_handler);
    app_run(app, 8080);
}
```

Route registration currently supports:

```c
app_get(app, "/products", products_handler);
app_post(app, "/products", create_product_handler);
```

The method must match exactly. `POST` routes can receive a request body; `request->body` is a NUL-terminated copy of the bytes after the blank line in the HTTP request.

## Reading request data

`http_request_t`, declared in [`include/http.h`](include/http.h), contains the data parsed for a handler:

| Field | Purpose |
| --- | --- |
| `request->http_request_line->method` | HTTP method, for example `GET` |
| `request->http_request_line->path` | Path without the query string |
| `request->http_request_line->version` | HTTP version from the request line |
| `request->http_request_line->query_string` | Raw text after `?`, or `NULL` |
| `request->headers` / `header_count` | Parsed header name/value pairs |
| `request->body` | Request body, or `NULL` for an empty body |
| `request->query_params` | Parsed query parameters |
| `request->params` | Values captured from a parameterized route |

### Path parameters

Prefix a path segment with `:` to capture it:

```c
static http_response_t* show_user(http_request_t* request) {
    char* id = hashmap_get(request->params, "id");
    return http_text_response(id ? id : "missing id");
}

app_get(app, "/users/:id", show_user);
```

`GET /users/42` makes `id` available as `"42"`. Multiple parameters work too:

```c
app_get(app, "/users/:id/posts/:post_id", post_handler);
```

Static routes are checked first, so an exact registered path wins over a parameterized match.

### Query parameters

Query parameters are stored in the same internal hashmap interface:

```c
static http_response_t* search(http_request_t* request) {
    char* term = hashmap_get(request->query_params, "q");
    return http_text_response(term ? term : "no search term");
}

app_get(app, "/search", search);
```

For `GET /search?q=socket`, `term` is `"socket"`.

### Headers

Use `get_http_header_value` for an exact-name header lookup:

```c
char* user_agent = get_http_header_value(request, "User-Agent");
```

Header names are currently compared case-sensitively after parsing, so use the casing sent by the client or inspect `request->headers` yourself.

## Responses

Handlers should return one of the built-in response constructors:

```c
return http_text_response("plain text");
return http_json_response("{\"ok\":true}");
return http_html_response("<h1>Hello</h1>");
return http_not_found_response("Nothing here");
```

They produce the following defaults:

| Helper | Status | Content type |
| --- | --- | --- |
| `http_text_response` | `200 OK` | `text/plain` |
| `http_json_response` | `200 OK` | `application/json` |
| `http_html_response` | `200 OK` | `text/html` |
| `http_not_found_response` | `404 Not Found` | `text/plain` |

Each helper adds `Content-Type` and a byte count in `Content-Length`. The framework serializes and frees the response after the handler returns, so handlers should not free a successful response themselves.

## Middleware

Middleware runs in registration order after route matching and before the handler. Its signature is:

```c
typedef bool (*middleware_handler)(
    http_request_t* request,
    http_response_t** response
);
```

Return `true` to continue. To stop the chain, assign a response through `response` and return `false`:

```c
static bool request_logger(http_request_t* request, http_response_t** response) {
    (void)response;
    printf("%s %s\n",
           request->http_request_line->method,
           request->http_request_line->path);
    return true;
}

static bool block_secret(http_request_t* request, http_response_t** response) {
    if (strcmp(request->http_request_line->path, "/secret") == 0) {
        *response = http_not_found_response("Unavailable");
        return false;
    }
    return true;
}

app_use(app, request_logger);
app_use(app, block_secret);
```

Middleware is not invoked for an unmatched route: those requests receive the framework's 404 response first.

## Templates

`render_template` loads a file relative to `templates/` and returns it as an HTML response:

```c
#include "template.h"

static http_response_t* users_page(http_request_t* request) {
    (void)request;
    return render_template("index.html");
}
```

The current renderer serves file contents as-is; it does not interpolate variables or evaluate template syntax. It rejects empty filenames, absolute paths, `..`, and backslashes. A missing file returns `404 Template not found`.

## How a request moves through the project

```text
client TCP connection
        |
connection_receive_request()      reads through headers + Content-Length body
        |
http_request_create()             parses line, headers, body, query parameters
        |
route_find()                      matches method/path and captures :parameters
        |
app_use() middleware chain        may continue or return a response
        |
route handler                     creates a response
        |
http_response_serialize()         writes HTTP/1.1 response to the socket
```

## Project layout

```text
include/                 Public types and function declarations
src/server.c             Runnable example application and routes
src/app*.c               App setup, route registration, middleware, server loop
src/connection.c         Socket receive loop and Content-Length handling
src/http_*.c             HTTP parsing, utilities, and response serialization
src/route.c              Static and :parameter route matching
src/libraries/hashmap.c  Internal FNV-1a chained hashmap
src/template.c           File-backed HTML response helper
templates/               HTML files served by render_template()
Makefile                 GCC build for compiled/server
```

## Current limitations

This project is intentionally at the "learn the primitives" stage. In particular:

- The server handles one client connection at a time and has no concurrency model.
- It has no TLS, keep-alive support, request timeout, request-size limit, or graceful shutdown.
- Only `Content-Length` bodies are handled; chunked transfer encoding is not supported.
- HTTP methods are routed only when registered with `app_get` or `app_post`.
- Query strings are basic `key=value&key=value` parsing: URL decoding, repeated keys, and key-only parameters are not implemented.
- Responses are text-based and use `strlen`, so binary response bodies are not supported.
- The public response API currently provides the standard helpers above; arbitrary status codes and custom response headers are internal capabilities rather than public documented APIs.
- Template rendering is file serving, not a templating language.
- There is no test suite yet, and error handling is still being expanded.

Treat it as a local development and learning server, not an Internet-facing service.

## Building and debugging

The default build command is:

```bash
make
```

It compiles `compiled/server` with warnings enabled and AddressSanitizer (`-fsanitize=address`). This is especially helpful while changing allocation-heavy code. If AddressSanitizer reports an issue, it will print a stack trace when the program exits or detects the error.

The Makefile has two targets:

```bash
make        # build compiled/server
make clean  # remove compiled/server
```

Run commands from the repository root. Template paths are relative (`templates/<filename>`), so launching the executable from another working directory will prevent templates from being found.

## Good next learning steps

If you want to keep evolving the project, these are natural, contained improvements:

1. Add a small test harness for HTTP parsing, routes, and response serialization.
2. Fix route storage growth and make all allocation-failure paths robust.
3. Expose a safe public API for custom response status codes and headers.
4. Add URL decoding and case-insensitive request-header lookup.
5. Add request limits, timeouts, and explicit `Connection: close` behavior.
6. Refactor the blocking loop into a concurrent or event-driven server.
7. Add JSON/body parsing and template contexts only after the lower-level behavior is tested.

## License

No license file is currently included. Add one before distributing or accepting contributions so the intended permissions are clear.
