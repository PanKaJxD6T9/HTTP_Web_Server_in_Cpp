#include "router.h"

#include <fstream>
#include <utility>

Router::Router(std::string publicDirectory)
    : publicDirectory_(std::move(publicDirectory)) {
}

bool Router::isKnownRoute(const std::string& path) const {
    return !resolveRoute(path).empty();
}

std::string Router::resolveRoute(const std::string& path) const {
    if (path == "/") {
        return publicDirectory_ + "/index.html";
    }

    const std::string normalizedPath = normalizePath(path);
    if (normalizedPath.empty()) {
        return "";
    }

    const std::string directPath = publicDirectory_ + "/" + normalizedPath;
    if (fileExists(directPath)) {
        return directPath;
    }

    if (normalizedPath.size() < 5 || normalizedPath.substr(normalizedPath.size() - 5) != ".html") {
        const std::string htmlFallbackPath = directPath + ".html";
        if (fileExists(htmlFallbackPath)) {
            return htmlFallbackPath;
        }
    }

    return "";
}

std::string Router::contentTypeForPath(const std::string& path) const {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") {
        return "text/html; charset=UTF-8";
    }

    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") {
        return "text/css; charset=UTF-8";
    }

    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") {
        return "application/javascript; charset=UTF-8";
    }

    return "text/plain; charset=UTF-8";
}

std::string Router::normalizePath(const std::string& path) const {
    if (path.empty() || path[0] != '/') {
        return "";
    }

    const std::string normalizedPath = path.substr(1);
    if (normalizedPath.find("..") != std::string::npos) {
        return "";
    }

    return normalizedPath;
}

bool Router::shouldRedirectToCleanRoute(const std::string& path) const {
    return !cleanRouteForPath(path).empty();
}

std::string Router::cleanRouteForPath(const std::string& path) const {
    const std::string normalizedPath = normalizePath(path);
    if (normalizedPath.empty()) {
        return "";
    }

    if (normalizedPath == "index.html") {
        return "/";
    }

    if (normalizedPath.size() < 5 || normalizedPath.substr(normalizedPath.size() - 5) != ".html") {
        return "";
    }

    const std::string directPath = publicDirectory_ + "/" + normalizedPath;
    if (!fileExists(directPath)) {
        return "";
    }

    return "/" + normalizedPath.substr(0, normalizedPath.size() - 5);
}

bool Router::fileExists(const std::string& filePath) const {
    std::ifstream file(filePath);
    return file.good();

}
