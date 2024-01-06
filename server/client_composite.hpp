#pragma once

#include <functional>
#include <list>
#include <mutex>
#include <ranges>

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

    ClientHandler& Emplace(Socket&& header_socket, SocketStream&& payload_stream, Socket&& sync_sc_socket,
                           Socket&& sync_cs_socket) noexcept(false);

    const std::string& GetUsername() const noexcept override { return username_; }

    bool BroadcastCommand(const std::function<bool(ClientHandler&, const std::filesystem::path&)>& method,
                          ClientHandler::IdType origin, const std::filesystem::path& path);

    bool BroadcastUpload(ClientHandler::IdType origin, const std::filesystem::path& path) override {
        static const std::function<bool(ClientHandler&, const std::filesystem::path&)> kMethod =
            &ClientHandler::SyncUpload;
        return BroadcastCommand(kMethod, origin, path);
    }

    bool BroadcastDelete(ClientHandler::IdType origin, const std::filesystem::path& path) override {
        static const std::function<bool(ClientHandler&, const std::filesystem::path&)> kMethod =
            &ClientHandler::SyncDelete;
        return BroadcastCommand(kMethod, origin, path);
    }

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
