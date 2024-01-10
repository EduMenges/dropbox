#pragma once

#include <functional>
#include <list>
#include <mutex>
#include <ranges>

#include "ClientHandler.hpp"
#include "CompositeInterface.hpp"
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

    /// Builds a new client at the end of list.
    dropbox::ClientHandler& Emplace(Socket&& payload_socket, Socket&& client_sync, Socket&& server_sync,
                                    SocketStream&& payload_stream) noexcept(false);

    [[nodiscard]] const std::string& GetUsername() const noexcept override { return username_; }

    /**
     * Broadcasts a command to other replicas and client devices.
     * @param method The method to use with the other clients.
     * @param backup_method The method to use with all of the backup replicas.
     * @param origin The device that sent the broadcast.
     * @param path The path of the file.
     * @return Whether the broadcast was a success.
     */
    bool BroadcastCommand(const std::function<bool(ClientHandler&, const std::filesystem::path&)>& method,
                          const std::function<bool(BackupHandler&, const std::filesystem::path&)>& backup_method,
                          ClientHandler::IdType origin, const std::filesystem::path& path);

    bool BroadcastUpload(ClientHandler::IdType origin, const std::filesystem::path& path) override {
        return BroadcastCommand(&ClientHandler::SyncUpload, &BackupHandler::Upload, origin, path);
    }

    bool BroadcastDelete(ClientHandler::IdType origin, const std::filesystem::path& path) override {
        return BroadcastCommand(&ClientHandler::SyncDelete, &BackupHandler::Delete, origin, path);
    }

    void Remove(int id) override;

   private:
    static constexpr size_t kDeviceLimit = 2U;  ///< Maximum amount of devices that one client can connect with.

    std::string                 username_; ///< Username of the client.
    std::list<ClientHandler>    list_;   ///< List of the devices for one client.
    std::mutex                  mutex_;  ///< Mutex for handling the list.
    std::vector<BackupHandler>& backups_; ///< Reference to the backup replicas.
};
}
