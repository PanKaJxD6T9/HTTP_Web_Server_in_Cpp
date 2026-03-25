#ifndef SERVER_H
#define SERVER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#include <string>

#include "router.h"

class HttpServer {
public:
    HttpServer(int port, std::string publicDirectory);
    void start() const;

private:
    int port_;
    Router router_;

    SOCKET createServerSocket() const;
    void handleClient(SOCKET clientSocket) const;
    static DWORD WINAPI handleClientThread(LPVOID param);
    std::string buildResponseForRequest(const std::string& rawRequest) const;
};

#endif
