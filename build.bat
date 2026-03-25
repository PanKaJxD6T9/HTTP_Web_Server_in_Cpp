@echo off
setlocal

echo Building HTTP server...
g++ -std=c++17 -Wall -Wextra -pedantic src\main.cpp src\parser.cpp src\router.cpp src\response.cpp src\server.cpp -o server.exe -lws2_32

if errorlevel 1 (
    echo.
    echo Build failed.
    exit /b 1
)

echo.
echo Build succeeded. Output: server.exe
endlocal
