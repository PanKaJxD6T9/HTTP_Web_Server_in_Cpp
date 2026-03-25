#include "parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace {
std::string trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string normalizeHeaderName(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return name;
}
}

HttpRequest HttpParser::parseRequest(const std::string& rawRequest) {
    const std::size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        throw std::runtime_error("Failed to find end of HTTP headers");
    }

    const std::string headerSection = rawRequest.substr(0, headerEnd);
    std::istringstream requestStream(headerSection);
    HttpRequest request;
    std::string line;

    if (!(requestStream >> request.method >> request.path >> request.version)) {
        throw std::runtime_error("Failed to parse HTTP request line");
    }

    std::getline(requestStream, line);

    while (std::getline(requestStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        const std::size_t separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        request.headers[normalizeHeaderName(trim(line.substr(0, separator)))] =
            trim(line.substr(separator + 1));
    }

    request.body = rawRequest.substr(headerEnd + 4);
    return request;
}
