#include <charconv>
#include <csignal>
#include <cstring>

#include <iostream>
#include <variant>

#include "Server.hpp"
#include "fmt/core.h"
#include "networking/Addr.hpp"
#include "toml++/toml.hpp"

namespace dropbox {
enum ArgV : size_t { kExecutionPath [[maybe_unused]] = 0U, kIndex, kTotal };

std::optional<std::vector<Addr>> ParseConfig() {
    try {
        toml::table       config;
        std::vector<Addr> server_collection;
        config       = toml::parse_file("config.toml");
        auto servers = *config["servers"].as_array();

        for (size_t i = 0; i < servers.size(); ++i) {
            auto       &server = servers[i];
            std::string ip     = std::string((*server.as_table())["ip"].as_string()->get());
            auto        port   = static_cast<in_port_t>((*server.as_table())["port"].as_integer()->get());

            server_collection.emplace_back(static_cast<Addr::IdType>(i), std::move(ip), port);
        }

        return server_collection;
    } catch (const toml::parse_error &err) {
        fmt::println(stderr, "Parsing failed: {}", err.what());
        return std::nullopt;
    }
}
}

using enum dropbox::ArgV;
using dropbox::ArgV, dropbox::replica::MainLoopReply;

/// Whether to shutdown
std::atomic<bool> should_stop = false;

extern "C" {
static void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        should_stop = true;
    }
}
}

int main(int argc, char *argv[]) {
    if (static_cast<ArgV>(argc) != kTotal) {
        fmt::println(stderr, "Usage: <index>");
        return EXIT_FAILURE;
    }

    size_t index   = 0;
    auto [ptr, ec] = std::from_chars(argv[kIndex], argv[kIndex] + strlen(argv[kIndex]), index);

    if (ec != std::errc()) {
        fmt::println(stderr, "Failure converting index \"{}\"", argv[kIndex]);
        return EXIT_FAILURE;
    }

    struct sigaction sa {};
    sa.sa_handler = SignalHandler;
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, nullptr) == -1 || sigaction(SIGTERM, &sa, nullptr) == -1) {
        perror("Error setting up SIGINT handler");
        return EXIT_FAILURE;
    }

    auto parsed_config = dropbox::ParseConfig();

    if (!parsed_config.has_value()) {
        return EXIT_FAILURE;
    }

    auto &servers = *parsed_config;

    try {
        dropbox::Server server(index, std::vector(servers), should_stop);

        tl::expected<dropbox::Addr::IdType, dropbox::Election::Error> election_result;
        dropbox::replica::MainLoopReply                               loop_result;
        size_t                                                        offset = 1;

        do {
            bool has_connection_to_next = server.HasConnectionToNext();

            if (!has_connection_to_next) {
                server.CreateNext();
                has_connection_to_next = server.ConnectNext(offset);
            }

            if (has_connection_to_next) {
                fmt::println("Election: next has connection");

                while (!server.GetRing().HasPrev()) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                fmt::println("Election: previous has connection");

                election_result = server.PerformElection();
            } else {
                fmt::println("Election: no servers found, skipping election");
                election_result = server.GetId();
            }

            if (election_result.has_value()) {
                loop_result = server.HandleElection(*election_result);

                if (loop_result == dropbox::replica::MainLoopReply::kLostConnectionToPrimary) {
                    offset += 1;
                }
            } else {
                fmt::println(stderr, "Election: {}", election_result.error());
            }
        } while (loop_result != dropbox::replica::MainLoopReply::kShutdown);

    } catch (std::exception &e) {
        fmt::println(stderr, "{}", e.what());
    }

    return EXIT_SUCCESS;
}
