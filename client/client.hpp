#pragma once

#include <netinet/in.h>

#include <communication/protocol.hpp>
#include <cstdint>
#include <filesystem>
#include <string>
#include <thread>

#include "utils.hpp"

#include "../common/inotify.hpp"

namespace dropbox {

/// Client class containing all the methods that the application shall provide.
class Client {
   public:
    /**
     * @param user_name Name of the user.
     * @param server_ip_address IP of the server.
     * @param port Port that the server is listening to.
     * @pre \p server_ip_address is IPV4.
     */
    Client(std::string&& user_name, const char* server_ip_address, in_port_t port);

    /// Clients are not copiable due to side effect in socket closing.
    Client(const Client& other) = delete;

    Client(Client&& other) = default;

    ~Client();

    /**
     * Sends the username to the server.
     * @return Status of the operation.
     */
    bool SendUsername();

    /**
     * Downloads the directory and start sync with the server.
     * @return Status of the operation.
     */
    bool GetSyncDir();

    /**
     * Lists the files available on the client's \c sync_dir.
     * @return Status of the operation.
     */
    bool ListClient() const;

    /**
     * Lists the files available on the client's \c sync_dir.
     * @return Status of the operation.
     */
    bool ListServer();

    /**
     * Downloads a file from the server into \c cwd.
     * @param file_name File to be downloaded.
     * @return Status of the operation.
     */
    bool Download(std::filesystem::path&& file_name);

    /**
     * Deletes a file from the server.
     * @param file_name File to be deleted.
     * @return Status of the operation.
     */
    bool Delete(std::filesystem::path&& file_name);

    /**
     * Ends main loop and communication with server.
     * @return Status of the operation.
     */
    bool Exit();

    bool ReceiveSyncFromServer();

    /**
     * @return Sync dir path concatenated with the username.
     */
    [[nodiscard]] inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(username_); }

    /**
     * @brief Uploads a file to the server.
     * @pre Assumes that \p path is a valid file.
     */
    bool Upload(std::filesystem::path&& path);

   private:
    std::string username_;       ///< User's name, used as an identifier.
    int         header_socket_;  ///< Socket to exchange headers.
    int         file_socket_;    ///< Socket to exchange files.

    int         sync_sc_socket_;    ///< Socket only for sync server -> client
    int         sync_cs_socket_;    ///< Socket only for sync client -> server

    HeaderExchange    he_; ///< What to exchange headers (commands) to the server with.
    FileExchange      fe_; ///< What to exchange files with the server with.

    HeaderExchange sche_;
    FileExchange   scfe_;

    HeaderExchange cshe_;
    FileExchange   csfe_;

    Inotify inotify_;
    std::thread inotify_client_thread_;

};

}
