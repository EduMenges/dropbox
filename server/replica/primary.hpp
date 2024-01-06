#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>

#include <thread>
#include <vector>

#include "connections.hpp"
#include "fmt/core.h"
#include "networking/socket.hpp"

namespace dropbox {
class PrimaryReplica {
   public:
    PrimaryReplica(const std::string &ip);

    PrimaryReplica(PrimaryReplica &&other) = default;

    PrimaryReplica(const PrimaryReplica &other) = delete;

    ~PrimaryReplica() = default;

    bool Accept() {
        Socket new_backup = accept(receiver_, nullptr, nullptr);

        if (!new_backup.IsValid()) {
            return false;
        }

        fmt::println("New server");
        backups_.push_back(std::move(new_backup));
        return true;
    }

    void AcceptLoop() {
        accept_thread_ = std::jthread([&](const std::stop_token &stop_token) {
            while (!stop_token.stop_requested()) {
                Accept();
            }
        });
    }

    std::vector<Socket> backups_;

    static constexpr in_port_t kAdminPort = 12345;
   private:
    static constexpr timeval   kTimeout{1, 0};
    Socket                     receiver_;
    std::jthread               accept_thread_;
};
}