#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>

class HttpResponseBuilder {
public:
    static std::string ok(const std::string& body, const std::string& contentType, bool includeBody = true);
    static std::string redirect(const std::string& location, bool includeBody = true);
    static std::string notFound(bool includeBody = true);
    static std::string methodNotAllowed(bool includeBody = true);
    static std::string badRequest(bool includeBody = true);

private:
    static std::string buildResponse(
        const std::string& statusLine,
        const std::string& body,
        const std::string& contentType,
        const std::string& extraHeaders = "",
        bool includeBody = true
    );
};

std::string readFileContents(const std::string& filePath);

#endif
