#include "parser.h"

#include <sstream>
#include <stdexcept>

HttpRequest HttpParser::parseRequestLine(const std::string& rawRequest) {
    std::istringstream requestStream(rawRequest);
    HttpRequest request;

    // Only the first line is needed for this simple GET-only server.
    if (!(requestStream >> request.method >> request.path >> request.version)) {
        throw std::runtime_error("Failed to parse HTTP request line");
    }

    return request;
}
