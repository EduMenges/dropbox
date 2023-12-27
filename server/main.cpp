#include <charconv>
#include <cstring>
#include <csignal>

#include <iostream>

#include "server.hpp"

namespace dropbox {
enum ArgV : size_t { kExecutionPath [[maybe_unused]] = 0U, kPort, kTotal };
}

using enum dropbox::ArgV;
using dropbox::ArgV;

static void SignalHandler(int signal) {
    std::cout << "Shuting down\n";
}

int main(int argc, char* argv[]) {
    if (static_cast<ArgV>(argc) != kTotal) {
        std::cerr << "Usage: <port>\n";
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, SignalHandler) == SIG_ERR) {
        std::cerr << "Failure on setting sigal handler\n";
        return EXIT_FAILURE;
    }

    in_port_t port = 0;
    auto [ptr, ec] = std::from_chars(argv[kPort], argv[kPort] + strlen(argv[kPort]), port);

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
