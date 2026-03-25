#ifndef ROUTER_H
#define ROUTER_H

#include <string>

class Router {
public:
    explicit Router(std::string publicDirectory);

    std::string resolveRoute(const std::string& path) const;
    bool isKnownRoute(const std::string& path) const;
    std::string contentTypeForPath(const std::string& path) const;
    bool shouldRedirectToCleanRoute(const std::string& path) const;
    std::string cleanRouteForPath(const std::string& path) const;

private:
    std::string publicDirectory_;
    bool fileExists(const std::string& filePath) const;
    std::string normalizePath(const std::string& path) const;
};

#endif
