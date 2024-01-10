#pragma once

#include <atomic>
#include <csignal>
#include <netinet/in.h>

#include "networking/Socket.hpp"
#include "composite/Base.hpp"
#include "networking/SocketStream.hpp"
#include "replica/MainLoopReply.hpp"

namespace dropbox::replica {
class Backup {
   public:
    explicit Backup(const sockaddr_in& primary_addr);

    /**
     * Connects to the primary replica.
     * @return Success.
     */
    bool ConnectToPrimary() const { return exchange_.socket_.Connect(primary_addr_); }

    /**
     * Main loop of the backup.
     * @param shutdown Whether to shutdown the backup.
     * @return A reply on how the function ended.
     */
    MainLoopReply MainLoop(std::atomic_bool& shutdown);

    constexpr void SetPrimaryAddr(sockaddr_in primary_addr) { primary_addr_ = primary_addr; }

   private:
    sockaddr_in     primary_addr_;  ///< Address of the primary replica.
    composite::Base exchange_;      ///< How to exchange the server with.
};
}
