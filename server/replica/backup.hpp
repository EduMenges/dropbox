#pragma once

#include <atomic>
#include <csignal>
#include <netinet/in.h>

#include "networking/socket.hpp"
#include "composite/Base.hpp"
#include "networking/SocketStream.hpp"

namespace dropbox::replica {
class Backup {
   public:
    explicit Backup(const sockaddr_in& primary_addr);

    [[nodiscard]] bool ConnectToPrimary() const { return exchange_.socket_.Connect(primary_addr_); }

    void MainLoop(std::atomic_bool& shutdown);

   private:
    sockaddr_in     primary_addr_;
    composite::Base exchange_;
};
}
