#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "client_composite.hpp"

namespace dropbox {
/// Class that holds information for all of the clients and all of their devices.
class ClientPool {
   public:
    ClientPool()                        = default;

    ~ClientPool()                       = default;

    ClientPool(const ClientPool& other) = delete;

    ClientPool(ClientPool&& other)      = delete;

    /**
     * Inserts a client handler into its spot on the user's device list.
     * @param handler Handler to be inserted.
     * @return Instance of the inserted handler that is now OWNED by the composite.
     * @pre \p handler already knows its username.
     */
    ClientHandler& Insert(ClientHandler&& handler) noexcept(false);

   private:
    std::mutex mutex_;

    /// Collection that associates username with their devices.
    std::unordered_map<std::string, ClientComposite> clients_;
};
}
