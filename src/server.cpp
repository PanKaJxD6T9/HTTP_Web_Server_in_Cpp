#include "server.h"

#include <process.h>
#include <ws2tcpip.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "parser.h"
#include "response.h"

namespace {
constexpr int kBacklog = 10;
constexpr int kBufferSize = 4096;

struct ClientContext {
    const HttpServer* server;
    SOCKET clientSocket;
};

class WinsockSession {
public:
    WinsockSession() {
        WSADATA data{};
        const int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    ~WinsockSession() {
        WSACleanup();
    }
};
}

HttpServer::HttpServer(int port, std::string publicDirectory)
    : port_(port), router_(std::move(publicDirectory)) {
}

void HttpServer::start() const {
    WinsockSession winsockSession;
    const SOCKET serverSocket = createServerSocket();

    std::cout << ">>   " << "Server listening on port " << port_ << '\n';
    std::cout << ">>   " << "http://localhost:" << port_ << '\n';

    while (true) {
        sockaddr_in clientAddress{};
        int clientLength = sizeof(clientAddress);

        const SOCKET clientSocket = accept(
            serverSocket,
            reinterpret_cast<sockaddr*>(&clientAddress),
            &clientLength
        );

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to accept client: " << WSAGetLastError() << '\n';
            continue;
        }

        ClientContext* context = new ClientContext{this, clientSocket};
        const HANDLE threadHandle = CreateThread(
            nullptr,
            0,
            &HttpServer::handleClientThread,
            context,
            0,
            nullptr
        );

        if (threadHandle == nullptr) {
            std::cerr << "Failed to create client thread: " << GetLastError() << '\n';
            closesocket(clientSocket);
            delete context;
            continue;
        }

        CloseHandle(threadHandle);
    }
}

SOCKET HttpServer::createServerSocket() const {
    const SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create socket");
    }

    const char optionValue = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSocket, reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        throw std::runtime_error("Failed to bind socket to port");
    }

    if (listen(serverSocket, kBacklog) == SOCKET_ERROR) {
        closesocket(serverSocket);
        throw std::runtime_error("Failed to listen on socket");
    }

    return serverSocket;
}

void HttpServer::handleClient(SOCKET clientSocket) const {
    char buffer[kBufferSize] = {0};
    const int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        closesocket(clientSocket);
        return;
    }

    const std::string rawRequest(buffer, static_cast<std::size_t>(bytesRead));
    const std::string response = buildResponseForRequest(rawRequest);

    send(
        clientSocket,
        response.c_str(),
        static_cast<int>(response.size()),
        0
    );
    closesocket(clientSocket);
}

DWORD WINAPI HttpServer::handleClientThread(LPVOID param) {
    ClientContext* context = static_cast<ClientContext*>(param);
    context->server->handleClient(context->clientSocket);
    delete context;
    return 0;
}

std::string HttpServer::buildResponseForRequest(const std::string& rawRequest) const {
    try {
        const HttpRequest request = HttpParser::parseRequestLine(rawRequest);

        if (request.method != "GET") {
            return HttpResponseBuilder::methodNotAllowed();
        }

        if (router_.shouldRedirectToCleanRoute(request.path)) {
            return HttpResponseBuilder::redirect(router_.cleanRouteForPath(request.path));
        }

        if (!router_.isKnownRoute(request.path)) {
            return HttpResponseBuilder::notFound();
        }

        const std::string filePath = router_.resolveRoute(request.path);
        const std::string fileContents = readFileContents(filePath);
        return HttpResponseBuilder::ok(fileContents, router_.contentTypeForPath(filePath));
    } catch (const std::exception&) {
        return HttpResponseBuilder::badRequest();
    }
}
