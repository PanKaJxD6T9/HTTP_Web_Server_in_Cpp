#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
};

#endif
