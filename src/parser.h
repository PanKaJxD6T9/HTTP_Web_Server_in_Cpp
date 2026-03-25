#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "http_request.h"

class HttpParser {
public:
    static HttpRequest parseRequest(const std::string& rawRequest);
};

#endif
