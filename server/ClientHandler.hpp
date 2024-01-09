#pragma once

#include <filesystem>
#include <thread>

#include "communication/protocol.hpp"
#include "networking/SocketStream.hpp"
#include "composite/Sender.hpp"
#include "composite_interface.hpp"
#include "networking/socket.hpp"
#include "utils.hpp"

namespace dropbox {

/// Handles one device of a client.
class ClientHandler {
   public:
    using IdType = int;

    static constexpr IdType kInvalidId = kInvalidSocket;

    ClientHandler(CompositeInterface* composite, Socket&& payload_socket, Socket&& client_sync, Socket&& server_sync,
                  SocketStream&& payload_stream);

    /// Clients handlers are not copiable due to side effect in socket closing.
    ClientHandler(const ClientHandler& other) = delete;

    ClientHandler(ClientHandler&& other) noexcept;

    ~ClientHandler();

    /// Main loop of the handler (receiving orders from client and dealing with them).
    void MainLoop();

    /// Creates the sync_dir_directory on the server side if it does not exist.
    void CreateUserFolder() const { std::filesystem::create_directory(SyncDirPath()); }

    /// Username getter.
    [[nodiscard]] const std::string& GetUsername() const noexcept { return GetComposite()->GetUsername(); };

    /// Gets the parent composite structure.
    [[nodiscard]] inline CompositeInterface* GetComposite() const noexcept { return composite_; }

    /// Receives an upload from the client.
    bool Upload();

    /**
     * Provides a download for the client.
     * @return Whether the exchange was a success, not whether the file exists.
     */
    bool Download();

    /**
     * Deletes a file from the server's \c sync_dir.
     * @return Whether the operation was a success.
     */
    bool Delete();

    /**
     * Starts the synchronization tasks.
     * @return Operation status.
     */
    bool GetSyncDir();

    /**
     * Lists all the files on the server side along with their MAC times and sends them.
     * @return Operation status.
     */
    bool ListServer() noexcept;

    /**
     * Getter for the unique ID of the client.
     * @return Unique ID of the client.
     */
    [[nodiscard]] inline IdType GetId() const noexcept { return payload_socket_.Get(); }

    inline bool operator==(const ClientHandler& other) const noexcept { return GetId() == other.GetId(); }

    inline bool operator==(IdType id) const noexcept { return GetId() == id; }

    /**
     * Getter for the \c sync_dir path.
     * @return \c sync_dir path for this user.
     */
    [[nodiscard]] inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(GetUsername()); }

    bool SyncUpload(const std::filesystem::path& path);

    bool SyncDelete(const std::filesystem::path& path);

    void SyncFromClient(std::stop_token stop_token);

   private:
    /// How many attempts remain until a client is disconnected.
    static constexpr uint8_t kAttemptAmount = 5;

    CompositeInterface* composite_;  ///< Parent composite structure that OWNS this instance.

    Socket payload_socket_;
    Socket client_sync_;

    composite::Sender server_sync_composite_;

    SocketStream payload_stream_;
    SocketStream cs_stream_;

    FileExchange fe_;  ///< Exchanges files with the client.
    FileExchange csfe_;
};
}
