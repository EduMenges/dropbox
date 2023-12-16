#pragma once

#include <list>
#include <mutex>

#include "client_handler.hpp"
#include "composite_interface.hpp"

namespace dropbox {
/// Composite class to handle multiple clients of the same username.
class ClientComposite : public CompositeInterface {
   public:
    explicit ClientComposite(std::string&& username) : username_(std::move(username)){};

    ~ClientComposite() override = default;

    ClientComposite(const ClientComposite& other) = delete;

    ClientComposite(ClientComposite&& other) = delete;

    ClientHandler& Emplace(SocketType header_socket, SocketStream&& payload_stream, SocketType sync_sc_socket,
                           SocketType sync_cs_socket) noexcept(false);

    const std::string& GetUsername() const noexcept override { return username_; }

    /**
     * Removes from the list a client by its ID.
     * @param id Client ID.
     * @warning Destroys the client pointed to by \p id.
     */
    void Remove(int id) override;

   private:
    static constexpr size_t kDeviceLimit = 2U;  ///< Maximum amount of devices that one client can connect with.

    std::string              username_;
    std::list<ClientHandler> list_;   ///< List of the devices for one client.
    std::mutex               mutex_;  ///< Mutex for handling the list.
};
}
