#include "server.h"

#include <process.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
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

std::string readHttpRequest(SOCKET clientSocket) {
    std::string request;
    request.reserve(kBufferSize);

    while (true) {
        char buffer[kBufferSize] = {0};
        const int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead <= 0) {
            break;
        }

        request.append(buffer, static_cast<std::size_t>(bytesRead));

        const std::size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            continue;
        }

        std::size_t expectedBodyLength = 0;
        const std::size_t contentLengthPosition = request.find("Content-Length:");
        if (contentLengthPosition != std::string::npos && contentLengthPosition < headerEnd) {
            const std::size_t valueStart = contentLengthPosition + std::strlen("Content-Length:");
            const std::size_t valueEnd = request.find("\r\n", valueStart);
            expectedBodyLength = static_cast<std::size_t>(
                std::stoul(request.substr(valueStart, valueEnd - valueStart))
            );
        }

        if (request.size() >= headerEnd + 4 + expectedBodyLength) {
            break;
        }
    }

    return request;
}

std::string urlDecode(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            decoded.push_back(' ');
            continue;
        }

        if (value[i] == '%' && i + 2 < value.size()) {
            decoded.push_back(static_cast<char>(std::stoi(value.substr(i + 1, 2), nullptr, 16)));
            i += 2;
            continue;
        }

        decoded.push_back(value[i]);
    }

    return decoded;
}

std::string htmlEscape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (const char ch : value) {
        switch (ch) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }

    return escaped;
}

std::unordered_map<std::string, std::string> parseFormBody(const std::string& body) {
    std::unordered_map<std::string, std::string> fields;
    std::istringstream bodyStream(body);
    std::string pair;

    while (std::getline(bodyStream, pair, '&')) {
        const std::size_t separator = pair.find('=');
        const std::string key = urlDecode(pair.substr(0, separator));
        const std::string value = separator == std::string::npos
            ? ""
            : urlDecode(pair.substr(separator + 1));
        fields[key] = value;
    }

    return fields;
}

std::string buildContactSubmissionPage(const std::unordered_map<std::string, std::string>& fields) {
    const auto nameIt = fields.find("name");
    const auto emailIt = fields.find("email");
    const auto messageIt = fields.find("message");

    const std::string name = nameIt == fields.end() ? "Guest" : htmlEscape(nameIt->second);
    const std::string email = emailIt == fields.end() ? "Not provided" : htmlEscape(emailIt->second);
    const std::string message = messageIt == fields.end() ? "" : htmlEscape(messageIt->second);

    std::ostringstream page;
    page << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
         << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
         << "<title>Form Submitted</title>"
         << "<style>body{font-family:Segoe UI,sans-serif;background:#f5f1ea;color:#1f2933;margin:0;padding:32px;}"
         << ".card{max-width:640px;margin:0 auto;background:#fff;padding:24px;border-radius:16px;"
         << "box-shadow:0 16px 40px rgba(31,41,51,.12);}h1{margin-top:0;}dt{font-weight:700;margin-top:12px;}"
         << "dd{margin:4px 0 0;}a{color:#0f766e;text-decoration:none;}</style></head><body><main class=\"card\">"
         << "<h1>Thanks, " << name << ".</h1>"
         << "<p>Your form was submitted with a real HTTP POST request.</p><dl>"
         << "<dt>Email</dt><dd>" << email << "</dd>"
         << "<dt>Message</dt><dd>" << (message.empty() ? "No message provided." : message) << "</dd>"
         << "</dl><p><a href=\"/contact\">Submit another response</a></p></main></body></html>";
    return page.str();
}

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
    const std::string rawRequest = readHttpRequest(clientSocket);

    if (rawRequest.empty()) {
        closesocket(clientSocket);
        return;
    }

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
        const HttpRequest request = HttpParser::parseRequest(rawRequest);
        const bool isHeadRequest = request.method == "HEAD";

        if (request.method != "GET" && request.method != "HEAD" && request.method != "POST") {
            return HttpResponseBuilder::methodNotAllowed(!isHeadRequest);
        }

        if (router_.isApiRoute(request.path)) {
            return HttpResponseBuilder::ok(
                R"({"message": "Hello from C++ server"})",
                router_.contentTypeForPath(request.path),
                !isHeadRequest
            );
        }

        if (request.method == "POST" && request.path == "/contact") {
            const auto contentTypeIt = request.headers.find("content-type");
            if (contentTypeIt == request.headers.end()
                || contentTypeIt->second.find("application/x-www-form-urlencoded") == std::string::npos) {
                return HttpResponseBuilder::badRequest();
            }

            return HttpResponseBuilder::ok(
                buildContactSubmissionPage(parseFormBody(request.body)),
                "text/html; charset=UTF-8"
            );
        }

        if (request.method == "POST") {
            return HttpResponseBuilder::notFound();
        }

        if (router_.shouldRedirectToCleanRoute(request.path)) {
            return HttpResponseBuilder::redirect(router_.cleanRouteForPath(request.path), !isHeadRequest);
        }

        if (!router_.isKnownRoute(request.path)) {
            return HttpResponseBuilder::notFound(!isHeadRequest);
        }

        const std::string filePath = router_.resolveRoute(request.path);
        const std::string fileContents = readFileContents(filePath);
        return HttpResponseBuilder::ok(
            fileContents,
            router_.contentTypeForPath(filePath),
            !isHeadRequest
        );
    } catch (const std::exception&) {
        return HttpResponseBuilder::badRequest();
    }
}
