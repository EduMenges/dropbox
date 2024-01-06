#pragma once

#include <netinet/in.h>

#include "networking/socket.hpp"

namespace dropbox {
class BackupReplica {
   public:
    BackupReplica(const sockaddr_in kPrimaryAddr) : primary_addr_(kPrimaryAddr) {}

    [[nodiscard]] bool ConnectToPrimary() const { return socket_.Connect(primary_addr_); }

   private:
    sockaddr_in primary_addr_;
    Socket      socket_;
};
}
