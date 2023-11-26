#include <charconv>
#include <cstring>
#include <iostream>

#include "client.hpp"
#include "user_input.hpp"

namespace dropbox {
enum ArgV : size_t { EXECUTION_PATH [[maybe_unused]] = 0U, USER_NAME, SERVER_IP_ADDRESS, PORT, TOTAL };
}

using dropbox::ArgV;
using enum ArgV;

int main(int argc, char *argv[]) {  // NOLINT
    const bool kHasAllParameters = static_cast<ArgV>(argc) != TOTAL;
    if (kHasAllParameters) {
        std::cerr << "Usage: <username> <server_ip_address> <port>\n";
        return EXIT_FAILURE;
    }

    in_port_t port = 0;
    auto [ptr, ec] = std::from_chars(argv[PORT], argv[PORT] + strlen(argv[PORT]), port);

    const bool kSuccessOnConversion = ec == std::errc();
    if (kSuccessOnConversion) {
        try {
        dropbox::Client client(argv[USER_NAME], argv[SERVER_IP_ADDRESS], port);

        dropbox::UserInput input_reader(std::move(client));
        input_reader.Start();} catch (std::exception& e) {
            std::cerr << e.what() << '\n';
            perror(__func__);
        }
    } else {
        std::cerr << "Port conversion failed\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
