# Simple C++ HTTP Server

This project is a minimal HTTP web server written in C++ with Winsock sockets.

## Features

- Listens on port 8080
- Accepts client connections
- Parses HTTP GET requests
- Serves HTML files from the `public` folder
- Supports routes `/` and `/about`
- Redirects legacy `.html` page URLs to clean routes
- Returns `404 Not Found` for unknown routes
- Handles multiple clients with Windows threads

## Build

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp src/parser.cpp src/router.cpp src/response.cpp src/server.cpp -o server.exe -lws2_32
```

## Run

```powershell
.\server.exe
```

Then open:

- `http://localhost:8080/`
- `http://localhost:8080/about`
