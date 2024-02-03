#include <charconv>
#include <cstring>
#include <thread>

#include "fmt/core.h"
#include "client.hpp"
#include "user_input.hpp"

namespace dropbox {
enum ArgV : size_t {
    kExecutionPath [[maybe_unused]] = 0U,
    kUserName,
    kServerIpAddress,
    kPort,
    kTotal
};  // NOLINT(*-enum-size)
}

using dropbox::ArgV;
using enum ArgV;

int main(int argc, char* argv[]) {  // NOLINT
    const bool kHasAllParameters = static_cast<ArgV>(argc) == kTotal;
    if (!kHasAllParameters) {
        fmt::println(stderr, "Usage: <username> <server_ip_address> <port>");
        return EXIT_FAILURE;
    }

    in_port_t port = 0;
    auto [ptr, ec] = std::from_chars(argv[kPort], argv[kPort] + strlen(argv[kPort]), port);

    const bool kSuccessOnConversion = ec == std::errc();
    if (!kSuccessOnConversion) {
        fmt::println(stderr, "Could not convert port \"{}\" to string", argv[kPort]);
        return EXIT_FAILURE;
    }

    try {
        dropbox::Client client(argv[kUserName], argv[kServerIpAddress], port);

        std::jthread inotify_thread([&client](auto stop_token) { client.StartInotify(stop_token); });

        std::jthread file_exchange_thread([&client](auto stop_token) { client.SyncFromClient(stop_token); });

        std::jthread sync_thread([&client](auto stop_token) { client.SyncFromServer(stop_token); });

        dropbox::UserInput(client).Start();

        sync_thread.request_stop();

        file_exchange_thread.request_stop();

        inotify_thread.request_stop();
    } catch (std::exception& e) {
        fmt::println("{}", e.what());
    }

/*
    bool can_continue;
    std::string server_ip = argv[kServerIpAddress];

    dropbox::Client client(argv[kUserName], server_ip.c_str(), port);

    std::jthread inotify_thread([&client](auto stop_token) { client.StartInotify(stop_token); });
    std::jthread file_exchange_thread([&client](auto stop_token) { client.SyncFromClient(stop_token); });
    std::jthread sync_thread([&client](auto stop_token) { client.SyncFromServer(stop_token); });

    can_continue =  dropbox::UserInput(client).Start();
    server_ip = client.elected_ip_;

    sync_thread.request_stop();
    file_exchange_thread.request_stop();
    inotify_thread.request_stop();


    if (!can_continue) {
        fmt::println("saindo");
        return EXIT_SUCCESS;
    }


    dropbox::Client client1(argv[kUserName], server_ip.c_str(), port);

    std::jthread inotify_thread1([&client1](auto stop_token) { client1.StartInotify(stop_token); });
    std::jthread file_exchange_thread1([&client1](auto stop_token) { client1.SyncFromClient(stop_token); });
    std::jthread sync_thread1([&client1](auto stop_token) { client1.SyncFromServer(stop_token); });

    can_continue =  dropbox::UserInput(client1).Start();
    server_ip = client1.elected_ip_;

    sync_thread1.request_stop();
    file_exchange_thread1.request_stop();
    inotify_thread1.request_stop();

*/
    fmt::println("saindo");
    return EXIT_SUCCESS;
}
