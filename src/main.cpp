#include <exception>
#include <iostream>

#include "server.h"

int main() {
    try {
        HttpServer server(5555, "public");
        server.start();
    } catch (const std::exception& exception) {
        std::cerr << "Server error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
