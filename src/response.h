#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>

class HttpResponseBuilder {
public:
    static std::string ok(const std::string& body, const std::string& contentType);
    static std::string redirect(const std::string& location);
    static std::string notFound();
    static std::string methodNotAllowed();
    static std::string badRequest();

private:
    static std::string buildResponse(
        const std::string& statusLine,
        const std::string& body,
        const std::string& contentType,
        const std::string& extraHeaders = ""
    );
};

std::string readFileContents(const std::string& filePath);

#endif
