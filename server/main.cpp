#include <netinet/in.h>

#include <charconv>
#include <cstring>
#include <iostream>

#include "server.hpp"

namespace dropbox {
enum ArgV : size_t { EXECUTION_PATH [[maybe_unused]] = 0U, PORT, TOTAL };
}

using namespace dropbox;

int main(int argc, char *argv[]) {
    if (static_cast<ArgV>(argc) != TOTAL) {
        std::cerr << "Usage: <port>\n";
        return EXIT_FAILURE;
    }

    in_port_t port = 0;
    auto [ptr, ec] =
        std::from_chars(argv[PORT], argv[PORT] + strlen(argv[PORT]), port);

    if (ec == std::errc()) {
        Server server(port);
        server.MainLoop();
    } else {
        std::cerr << "Port conversion failed\n";
        return EXIT_FAILURE;
    }
}
