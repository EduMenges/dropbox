#pragma once

#include <list>
#include <mutex>

#include "composite_interface.hpp"
#include "client_handler.hpp"

namespace dropbox {
/// Composite class to handle multiple clients of the same username.
class ClientComposite : public CompositeInterface {
   public:
    ClientComposite() = default;

    ~ClientComposite() override = default;

    ClientComposite(const ClientComposite& other) = delete;

    ClientComposite(ClientComposite&& other) = delete;

    /**
     * Inserts a new client.
     * @param client Client instance to be inserted.
     * @throws @ref FullList if list is full.
     * @return Reference to the inserted client instance.
     */
    ClientHandler& Insert(ClientHandler&& client) noexcept(false);

    /**
     * Removes from the list a client by its ID.
     * @param id Client ID.
     * @warning Destroys the client pointed to by \p id.
     */
    void Remove(int id) override;

   private:
    static constexpr size_t kDeviceLimit = 2U; ///< Maximum amount of devices that one client can connect with.

    std::list<ClientHandler> list_; ///< List of the devices for one client.
    std::mutex               mutex_; ///< Mutex for handling the list.
};
}
