#pragma once

#include <iostream>
#include <thread>

#include "communication/protocol.hpp"
#include "communication/socket_stream.hpp"
#include "composite_interface.hpp"
#include "utils.hpp"

namespace dropbox {

/// Handles one device of a client.
class ClientHandler {
   public:
    using IdType = int;

    static constexpr IdType kInvalidId = kInvalidSocket;

    ClientHandler(CompositeInterface* composite, int header_socket, SocketStream&& payload_stream, int sync_sc_socket, int sync_cs_socket);

    /// Clients handlers are not copiable due to side effect in socket closing.
    ClientHandler(const ClientHandler& other) = delete;

    ClientHandler(ClientHandler&& other) noexcept;

    ~ClientHandler();

    /// Main loop of the handler (receiving orders from client and dealing with them).
    void MainLoop();

    /// Creates the sync_dir_directory on the server side if it does not exist.
    void CreateUserFolder() const;

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
    [[nodiscard]] inline IdType GetId() const noexcept { return header_socket_; }

    inline bool operator==(const ClientHandler& other) const noexcept { return GetId() == other.GetId(); }

    inline bool operator==(IdType id) const noexcept { return GetId() == id; }

    /**
     * Getter for the \c sync_dir path.
     * @return \c sync_dir path for this user.
     */
    [[nodiscard]] inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(GetUsername()); }

    bool SyncUpload(const std::filesystem::path& path);

    bool SyncDelete(const std::filesystem::path& path);

    void SyncFromClient();

   private:
    /// How many attempts remain until a client is disconnected.
    static constexpr uint8_t kAttemptAmount = 5;

    CompositeInterface* composite_;  ///< Parent composite structure that OWNS this instance.

    int         header_socket_;  ///< Socket to exchange the header with.
    int         sync_sc_socket_;
    int         sync_cs_socket_;

    SocketStream payload_stream_;
    SocketStream sc_stream_;
    SocketStream cs_stream_;

    HeaderExchange he_;  ///< Exchanges headers with the client.
    FileExchange   fe_;  ///< Exchanges files with the client.

    FileExchange   scfe_;

    FileExchange   csfe_;

    bool server_sync_;
};
}
