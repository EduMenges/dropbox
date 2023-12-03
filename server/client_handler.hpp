#pragma once

#include <iostream>
#include <thread>

#include "communication/protocol.hpp"
#include "composite_interface.hpp"
#include "utils.hpp"
#include "../common/inotify.hpp"

namespace dropbox {

/// Handles one device of a client.
class ClientHandler {
   public:
    /**
     * Constructor.
     * @param header_socket Socket to use in header communications.
     * @param file_socket Socket to use in file communications.
     * @pre Both \p header_socket and \p file_socket are initialized and connected to the client.
     */
    ClientHandler(int header_socket, int file_socket, int sync_sc_socket, int sync_cs_socket);

    using id_type = int;
    
    /// Clients handlers are not copiable due to side effect in socket closing.
    ClientHandler(const ClientHandler& other) = delete;

    ClientHandler(ClientHandler&& other) noexcept;

    ~ClientHandler();

    /// Main loop of the handler (receiving orders from client and dealing with them).
    void MainLoop();

    /// Creates the sync_dir_directory on the server side if it does not exist.
    void CreateUserFolder();

    /// Receives the username.
    bool ReceiveUsername();

    /// Username getter.
    [[nodiscard]] const std::string& GetUsername() const noexcept { return username_; };

    /// Sets the parent composite structure.
    inline void SetComposite(CompositeInterface* composite) noexcept { composite_ = composite; };

    /// Gets the parent composite structure.
    [[nodiscard]] inline CompositeInterface* GetComposite() const noexcept { return composite_; }

    /// Receives an upload from the client.
    bool ReceiveUpload();

    /**
     * Provides a download for the client.
     * @return Whether the exchange was a success, not whether the file exists.
     */
    bool ReceiveDownload();

    /**
     * Deletes a file from the server's \c sync_dir.
     * @return Whether the operation was a success.
     */
    bool ReceiveDelete();

    /**
     * Starts the synchronization tasks.
     * @return Operation status.
     */
    bool ReceiveGetSyncDir();

    void ReceiveSyncFromClient();

    /**
     * Lists all the files on the server side along with their MAC times and sends them.
     * @return Operation status.
     */
    bool ListServer() const;

    void StartInotify();

    void StartFileExchange();
    
    /**
     * Getter for the unique ID of the client.
     * @return Unique ID of the client.
     */
    [[nodiscard]] inline id_type GetId() const noexcept { return header_socket_; }

    inline bool operator==(const ClientHandler& other) const noexcept { return GetId() == other.GetId(); }

    inline bool operator==(id_type id) const noexcept { return GetId() == id; }

    /**
     * Getter for the \c sync_dir path.
     * @return \c sync_dir path for this user.
     */
    [[nodiscard]] inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(username_); }

   private:
    /// How many attempts remain until a client is disconnected.
    static constexpr uint8_t kAttemptAmount = 5;

    int         header_socket_; ///< Socket to exchange the header with.
    int         file_socket_;   ///< Socket to exchange files with.
    int         sync_sc_socket_;
    int         sync_cs_socket_;
    std::string username_;      ///< Username of the client.


    CompositeInterface* composite_;  ///< Parent composite structure that OWNS this instance.


    HeaderExchange he_;  ///< Exchanges headers with the client.
    FileExchange   fe_;  ///< Exchanges files with the client.

    HeaderExchange    sche_;
    FileExchange      scfe_;

    HeaderExchange    cshe_;
    FileExchange      csfe_;

    Inotify inotify_;

    bool server_sync_;
};
}
