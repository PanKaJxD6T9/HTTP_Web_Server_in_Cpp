#include "response.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string HttpResponseBuilder::ok(const std::string& body, const std::string& contentType) {
    return buildResponse("HTTP/1.1 200 OK", body, contentType);
}

std::string HttpResponseBuilder::redirect(const std::string& location) {
    const std::string body =
        "<html><body><h1>301 Moved Permanently</h1><p>Redirecting to the clean URL.</p></body></html>";
    return buildResponse(
        "HTTP/1.1 301 Moved Permanently",
        body,
        "text/html; charset=UTF-8",
        "Location: " + location + "\r\n"
    );
}

std::string HttpResponseBuilder::notFound() {
    const std::string body =
        "<html><body><h1>404 Not Found</h1><p>The requested route was not found.</p></body></html>";
    return buildResponse("HTTP/1.1 404 Not Found", body, "text/html; charset=UTF-8");
}

std::string HttpResponseBuilder::methodNotAllowed() {
    const std::string body =
        "<html><body><h1>405 Method Not Allowed</h1><p>Only GET is supported.</p></body></html>";
    return buildResponse("HTTP/1.1 405 Method Not Allowed", body, "text/html; charset=UTF-8");
}

std::string HttpResponseBuilder::badRequest() {
    const std::string body =
        "<html><body><h1>400 Bad Request</h1><p>Unable to parse the HTTP request.</p></body></html>";
    return buildResponse("HTTP/1.1 400 Bad Request", body, "text/html; charset=UTF-8");
}

std::string HttpResponseBuilder::buildResponse(
    const std::string& statusLine,
    const std::string& body,
    const std::string& contentType,
    const std::string& extraHeaders
) {
    std::ostringstream response;
    response << statusLine << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << extraHeaders;
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

std::string readFileContents(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
