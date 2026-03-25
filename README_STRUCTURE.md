# Project File Guide

This document explains what each file in the project does, why it exists, and how all parts work together.

## Project Layout

```text
Technical Training/
|-- build.bat
|-- Makefile
|-- README.md
|-- README_STRUCTURE.md
|-- public/
|   |-- index.html
|   |-- about.html
|   `-- index.css
`-- src/
    |-- main.cpp
    |-- http_request.h
    |-- parser.h
    |-- parser.cpp
    |-- router.h
    |-- router.cpp
    |-- response.h
    |-- response.cpp
    |-- server.h
    `-- server.cpp
```

## High-Level Idea

The program is a small HTTP web server written in C++. It:

- starts a server on port `8080`
- waits for browser connections
- reads the HTTP request
- parses the request line
- decides which file to serve
- builds an HTTP response
- sends that response back to the browser

The code is split into separate modules so each part has one clear job.

## Source Files and Headers

### `src/main.cpp`

This is the entry point of the program.

What it does:

- creates an `HttpServer` object
- tells it to listen on port `8080`
- passes `"public"` as the folder for static files
- starts the server
- catches fatal errors and prints them

Why it exists:

- keeps program startup simple
- leaves networking logic inside the server module

How it connects to the rest:

- includes `server.h`
- uses the `HttpServer` class

### `src/http_request.h`

This file defines the `HttpRequest` structure.

What it stores:

- `method` such as `GET`
- `path` such as `/` or `/index.css`
- `version` such as `HTTP/1.1`

Why it exists:

- gives the parser a clean data structure to return
- makes request information easy to pass around

How it connects to the rest:

- `parser.cpp` fills this struct
- `server.cpp` reads values from it

### `src/parser.h`

This header declares the `HttpParser` class.

What it provides:

- `parseRequestLine(const std::string& rawRequest)`

Why it exists:

- separates HTTP request parsing from socket code
- keeps parsing reusable and easy to understand

### `src/parser.cpp`

This file implements the request parser.

What it does:

- reads the first line of the raw HTTP request
- extracts the method, path, and version
- returns them inside an `HttpRequest`
- throws an error if parsing fails

Example:

If the browser sends:

```http
GET /index.css HTTP/1.1
Host: localhost:8080
```

The parser produces:

- `method = "GET"`
- `path = "/index.css"`
- `version = "HTTP/1.1"`

Why it exists:

- keeps text parsing out of `server.cpp`

### `src/router.h`

This header declares the `Router` class.

What it provides:

- `resolveRoute(...)`
- `isKnownRoute(...)`
- `contentTypeForPath(...)`
- a private helper `normalizePath(...)`

Why it exists:

- keeps route and file resolution logic in one place
- makes it easier to grow the server later

### `src/router.cpp`

This file decides which file should be served for a given URL path.

What it does:

- maps `/` to `public/index.html`
- maps `/about` to `public/about.html`
- allows direct static files like `/index.css` and `/about.html`
- blocks unsafe paths containing `..`
- checks whether the resolved file actually exists
- returns the correct content type for `.html`, `.css`, and `.js`

Why it exists:

- separates route decisions from response building
- keeps file path logic organized

Important behavior:

- `/` returns the home page
- `/about` returns the about page
- `/index.css` returns the CSS file
- unknown files return `404`

### `src/response.h`

This header declares the response builder functions.

What it provides:

- `ok(...)`
- `notFound()`
- `methodNotAllowed()`
- `badRequest()`
- `readFileContents(...)`

Why it exists:

- keeps HTTP response creation separate from routing and sockets

### `src/response.cpp`

This file builds full HTTP response strings.

What it does:

- creates a `200 OK` response for valid files
- creates a `404 Not Found` response for unknown routes
- creates a `405 Method Not Allowed` response for non-GET requests
- creates a `400 Bad Request` response when parsing fails
- adds headers like:
  - `Content-Type`
  - `Content-Length`
  - `Connection: close`
- reads file contents from disk

Why it exists:

- centralizes all HTTP response formatting
- avoids duplicating response-building code in the server

### `src/server.h`

This header declares the `HttpServer` class.

What it contains:

- the server port
- the `Router` object
- methods for creating the socket, handling clients, and building responses
- the thread entry function used by Windows threads

Why it exists:

- defines the public interface of the web server
- hides internal implementation details from `main.cpp`

### `src/server.cpp`

This file is the core of the application. It handles Winsock setup, listening, accepting clients, and sending responses.

What it does:

- initializes Winsock with `WSAStartup`
- creates a TCP socket
- binds the socket to port `8080`
- starts listening for incoming connections
- accepts browser connections in a loop
- creates a separate Windows thread for each client
- reads request bytes with `recv`
- parses the request
- uses the router to find the file
- uses the response builder to create the final HTTP response
- sends the response with `send`
- closes the client socket
- shuts down Winsock automatically when the server exits

Why it exists:

- it is the module that connects all the other modules together

## Public Folder

### `public/index.html`

This is the home page served for:

- `/`

It also links to:

- `index.css`
- `about.html`

### `public/about.html`

This is the about page served for:

- `/about`
- `/about.html`

### `public/index.css`

This file contains the styling for the site.

It is requested separately by the browser after `index.html` loads. That is why the server must support direct static file requests such as `/index.css`.

## Build Files

### `build.bat`

This is the Windows build script.

What it does:

- compiles all `.cpp` files with `g++`
- links against Winsock using `-lws2_32`
- creates `server.exe`

Why it exists:

- lets you build the project with one command on Windows

Run it with:

```powershell
.\build.bat
```

### `Makefile`

This is a traditional build file for `make`.

What it does:

- describes how to compile the project
- gives a `clean` target for removing the executable

Why it may not work on your machine:

- Windows PowerShell does not always include `make`
- your environment may need `mingw32-make`, MSYS2, or WSL

## How Everything Works Together

Here is the full request flow:

1. The program starts in `main.cpp`.
2. `main.cpp` creates `HttpServer`.
3. `HttpServer::start()` initializes Winsock and opens the listening socket.
4. A browser connects to `localhost:8080`.
5. The server accepts the connection.
6. A new Windows thread is created for that client.
7. The thread reads the raw HTTP request from the socket.
8. `HttpParser::parseRequestLine()` extracts the method, path, and version.
9. `Router` checks whether the path maps to a valid file.
10. If the file exists, `readFileContents()` loads it.
11. `HttpResponseBuilder::ok()` creates the HTTP response.
12. The response is sent back to the browser.
13. If the route is unknown, the server sends `404 Not Found`.
14. The client socket is closed.

## Example Request Walkthrough

Suppose the browser requests:

```http
GET /index.css HTTP/1.1
```

The modules work like this:

1. `server.cpp` receives the text from the socket.
2. `parser.cpp` extracts `GET`, `/index.css`, and `HTTP/1.1`.
3. `router.cpp` maps `/index.css` to `public/index.css`.
4. `response.cpp` reads the CSS file and builds:
   - status: `200 OK`
   - content type: `text/css`
5. `server.cpp` sends the response back to the browser.

## Why This Structure Is Good

This project is easier to understand because responsibilities are split clearly:

- `main.cpp` starts the app
- `server.cpp` handles sockets and client flow
- `parser.cpp` reads HTTP request data
- `router.cpp` decides which file to serve
- `response.cpp` builds HTTP responses
- `public/` stores website files

That makes the project easier to debug, extend, and explain.

## Possible Future Improvements

- support more HTTP methods
- serve images and other file types
- add query string parsing
- log requests to the console
- add graceful shutdown
- use a thread pool instead of one thread per client
- improve error messages for bind failures
