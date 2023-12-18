#include <charconv>
#include <cstring>
#include <iostream>
#include <thread>

#include "client.hpp"
#include "user_input.hpp"

namespace dropbox {
enum ArgV : size_t { kExecutionPath [[maybe_unused]] = 0U, kUserName, kServerIpAddress, kPort, kTotal };
}

using dropbox::ArgV;
using enum ArgV;

int main(int argc, char* argv[]) {  // NOLINT
    const bool kHasAllParameters = static_cast<ArgV>(argc) == kTotal;
    if (!kHasAllParameters) {
        std::cerr << "Usage: <username> <server_ip_address> <port>\n";
        return EXIT_FAILURE;
    }

    // Converting port from text to number
    in_port_t port = 0;
    auto [ptr, ec] = std::from_chars(argv[kPort], argv[kPort] + strlen(argv[kPort]), port);

    const bool kSuccessOnConversion = ec == std::errc();
    if (kSuccessOnConversion) {
        try {
            dropbox::Client client(argv[kUserName], argv[kServerIpAddress], port);

            std::jthread inotify_thread(
                [&client](auto stop_token) {
                client.StartInotify(stop_token);
                }
            );

            std::jthread file_exchange_thread(
                [&client](auto stop_token) { client.SyncFromClient(stop_token);
                }
            );

            std::jthread sync_thread(
                [&client](auto stop_token) {
                    client.SyncFromServer(stop_token);
                }
            );

            dropbox::UserInput(client).Start();

            file_exchange_thread.request_stop();

            inotify_thread.request_stop();

            sync_thread.request_stop();

        } catch (std::exception& e) {
            std::cerr << e.what() << '\n';
            perror(__func__);
        }
    } else {
        std::cerr << "Port conversion failed\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
