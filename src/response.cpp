#include "response.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string HttpResponseBuilder::ok(const std::string& body, const std::string& contentType, bool includeBody) {
    return buildResponse("HTTP/1.1 200 OK", body, contentType, "", includeBody);
}

std::string HttpResponseBuilder::redirect(const std::string& location, bool includeBody) {
    const std::string body =
        "<html><body><h1>301 Moved Permanently</h1><p>Redirecting to the clean URL.</p></body></html>";
    return buildResponse(
        "HTTP/1.1 301 Moved Permanently",
        body,
        "text/html; charset=UTF-8",
        "Location: " + location + "\r\n",
        includeBody
    );
}

std::string HttpResponseBuilder::notFound(bool includeBody) {
    const std::string body =
        "<html><body><h1>404 Not Found</h1><p>The requested route was not found.</p></body></html>";
    return buildResponse("HTTP/1.1 404 Not Found", body, "text/html; charset=UTF-8", "", includeBody);
}

std::string HttpResponseBuilder::methodNotAllowed(bool includeBody) {
    const std::string body =
        "<html><body><h1>405 Method Not Allowed</h1><p>Supported methods: GET, HEAD, POST.</p></body></html>";
    return buildResponse(
        "HTTP/1.1 405 Method Not Allowed",
        body,
        "text/html; charset=UTF-8",
        "Allow: GET, HEAD, POST\r\n",
        includeBody
    );
}

std::string HttpResponseBuilder::badRequest(bool includeBody) {
    const std::string body =
        "<html><body><h1>400 Bad Request</h1><p>Unable to parse the HTTP request.</p></body></html>";
    return buildResponse("HTTP/1.1 400 Bad Request", body, "text/html; charset=UTF-8", "", includeBody);
}

std::string HttpResponseBuilder::buildResponse(
    const std::string& statusLine,
    const std::string& body,
    const std::string& contentType,
    const std::string& extraHeaders,
    bool includeBody
) {
    std::ostringstream response;
    response << statusLine << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << extraHeaders;
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    if (includeBody) {
        response << body;
    }
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
