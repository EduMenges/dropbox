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

    bool Accept();

    void AcceptLoop();

    std::vector<Socket> backups_;

    static constexpr in_port_t kAdminPort = 12345;
   private:
    static constexpr timeval   kTimeout{1, 0};
    Socket                     receiver_;
    std::jthread               accept_thread_;
};
}