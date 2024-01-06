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

sig_atomic_t should_stop = 0;

extern "C" void SignalHandler(int signal) {
    if (signal == SIGINT) {
        should_stop = 1;
    }
}

int main(int argc, char* argv[]) {
    if (static_cast<ArgV>(argc) != kTotal) {
        std::cerr << "Usage: <port>\n";
        return EXIT_FAILURE;
    }

    in_port_t port = 0;
    auto [ptr, ec] = std::from_chars(argv[kPort], argv[kPort] + strlen(argv[kPort]), port);

    if (ec == std::errc()) {
        if (signal(SIGINT, SignalHandler) == SIG_ERR || signal(SIGTERM, SignalHandler) == SIG_ERR) {
            return EXIT_FAILURE;
        }

        try {
            dropbox::Server server(port);
            server.MainLoop(should_stop);
        } catch (std::exception& e) {
            std::cerr << e.what() << '\n';
            perror(__func__);
        }
    } else {
        std::cerr << "Port conversion failed\n";
        return EXIT_FAILURE;
    }
}
