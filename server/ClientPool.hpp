#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ClientComposite.hpp"
#include "ClientHandler.hpp"
#include "networking/SocketStream.hpp"
#include "networking/Socket.hpp"
#include "replica/BackupHandler.hpp"

namespace dropbox {
/// Class that holds information for all of the clients and all of their devices.
class ClientPool {
   public:
    ClientPool() = default;

    ~ClientPool() = default;

    ClientPool(const ClientPool& other) = delete;

    ClientPool(ClientPool&& other) = delete;

    dropbox::ClientHandler& Emplace(std::string&& username, std::vector<BackupHandler>& backups,
                                    Socket&& payload_socket, Socket&& client_sync, Socket&& server_sync,
                                    SocketStream&& payload_stream) noexcept(false);

   private:
    std::mutex mutex_;

    /// Collection that associates username with their devices.
    std::unordered_map<std::string, ClientComposite> clients_;
};
}
