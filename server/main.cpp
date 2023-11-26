#include <charconv>
#include <cstring>
#include <iostream>

#include "server.hpp"

namespace dropbox {
enum ArgV : size_t { EXECUTION_PATH [[maybe_unused]] = 0U, PORT, TOTAL };
}

using enum dropbox::ArgV;
using dropbox::ArgV;

int main(int argc, char* argv[]) {
    if (static_cast<ArgV>(argc) != TOTAL) {
        std::cerr << "Usage: <port>\n";
        return EXIT_FAILURE;
    }

    in_port_t port = 0;
    auto [ptr, ec] = std::from_chars(argv[PORT], argv[PORT] + strlen(argv[PORT]), port);

    if (ec == std::errc()) {
        try {
            dropbox::Server server(port);
            server.MainLoop();
        } catch (std::exception& e) {
            std::cerr << e.what() << '\n';
            perror(__func__);
        }
    } else {
        std::cerr << "Port conversion failed\n";
        return EXIT_FAILURE;
    }
}
