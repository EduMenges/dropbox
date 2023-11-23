#include <charconv>
#include <cstring>
#include <iostream>

#include "client.hpp"
#include "user_input.hpp"

namespace dropbox {
enum ArgV : size_t {
    EXECUTION_PATH [[maybe_unused]] = 0U,
    USER_NAME,
    SERVER_IP_ADDRESS,
    PORT,
    TOTAL
};
}

using dropbox::ArgV;
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
        dropbox::Client client(argv[USER_NAME], argv[SERVER_IP_ADDRESS], port);
        
        dropbox::UserInput inputReader(client);
        inputReader.Start();

    } else {
        std::cerr << "Port conversion failed\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
