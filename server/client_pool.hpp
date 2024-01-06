#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "client_composite.hpp"

namespace dropbox {
/// Class that holds information for all of the clients and all of their devices.
class ClientPool {
   public:
    ClientPool() = default;

    ~ClientPool() = default;

    ClientPool(const ClientPool& other) = delete;

    ClientPool(ClientPool&& other) = delete;

    ClientHandler& Emplace(std::string&& username, Socket&& header_socket, SocketStream&& payload_stream,
                           Socket&& sync_sc_socket, Socket&& sync_cs_socket) noexcept(false);

   private:
    std::mutex mutex_;

    /// Collection that associates username with their devices.
    std::unordered_map<std::string, ClientComposite> clients_;
};
}
