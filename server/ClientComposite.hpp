#pragma once

#include <functional>
#include <list>
#include <mutex>
#include <ranges>

#include "ClientHandler.hpp"
#include "composite_interface.hpp"
#include "replica/BackupHandler.hpp"

namespace dropbox {
/// Composite class to handle multiple clients of the same username.
class ClientComposite : public CompositeInterface {
   public:
    explicit ClientComposite(std::string&& username, std::vector<BackupHandler>& backups)
        : username_(std::move(username)), backups_(backups){};

    ~ClientComposite() override = default;

    ClientComposite(const ClientComposite& other) = delete;

    ClientComposite(ClientComposite&& other) = delete;

    dropbox::ClientHandler& Emplace(Socket&& payload_socket, Socket&& client_sync, Socket&& server_sync,
                                    SocketStream&& payload_stream) noexcept(false);

    [[nodiscard]] const std::string& GetUsername() const noexcept override { return username_; }

    bool BroadcastCommand(const std::function<bool(ClientHandler&, const std::filesystem::path&)>& method,
                          const std::function<bool(BackupHandler&, const std::filesystem::path&)>& backup_method,
                          ClientHandler::IdType origin, const std::filesystem::path& path);

    bool BroadcastUpload(ClientHandler::IdType origin, const std::filesystem::path& path) override {
        return BroadcastCommand(&ClientHandler::SyncUpload, &BackupHandler::Upload, origin, path);
    }

    bool BroadcastDelete(ClientHandler::IdType origin, const std::filesystem::path& path) override {
        return BroadcastCommand(&ClientHandler::SyncDelete, &BackupHandler::Delete, origin, path);
    }

    /**
     * Removes from the list a client by its ID.
     * @param id Client ID.
     * @warning Destroys the client pointed to by \p id.
     */
    void Remove(int id) override;

   private:
    static constexpr size_t kDeviceLimit = 2U;  ///< Maximum amount of devices that one client can connect with.

    std::string                 username_;
    std::list<ClientHandler>    list_;   ///< List of the devices for one client.
    std::mutex                  mutex_;  ///< Mutex for handling the list.
    std::vector<BackupHandler>& backups_;
};
}
