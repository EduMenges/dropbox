#include <charconv>
#include <cstring>
#include <iostream>

#include "client.hpp"

namespace dropbox {
enum ArgV : size_t {
    EXECUTION_PATH [[maybe_unused]] = 0U,
    USER_NAME,
    SERVER_IP_ADDRESS,
    PORT,
    TOTAL
};
}

using namespace dropbox;
using enum ArgV;

int main(int argc, char *argv[]) {  // NOLINT
    if (static_cast<ArgV>(argc) != TOTAL) {
        std::cerr << "Usage: <username> <server_ip_address> <port>\n";
        return EXIT_FAILURE;
    }

    in_port_t port = 0;
    auto [ptr, ec] =
        std::from_chars(argv[PORT], argv[PORT] + strlen(argv[PORT]), port);

    if (ec == std::errc()) {
        Client client(argv[USER_NAME], argv[SERVER_IP_ADDRESS], port);
    } else {
        std::cerr << "Port conversion failed\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
