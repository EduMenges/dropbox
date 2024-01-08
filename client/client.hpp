#pragma once

#include <netinet/in.h>

#include <communication/protocol.hpp>
#include <filesystem>
#include <string>

#include "networking/SocketStream.hpp"
#include "inotify.hpp"
#include "networking/socket.hpp"
#include "utils.hpp"

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
    Client(std::string&& user_name, const char* server_ip_address, in_port_t port) noexcept(false);

    /// Clients are not copiable due to side effect in socket closing.
    Client(const Client& other) = delete;

    Client(Client&& other) = delete;

    ~Client();

    /**
     * Sends the username to the server.
     */
    void SendUsername() noexcept(false);

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
    void Exit();

    void SyncFromServer(const std::stop_token& stop_token);

    void StartInotify(const std::stop_token& stop_token);

    void SyncFromClient(std::stop_token stop_token);

    /**
     * @return Sync dir path concatenated with the username.
     */
    [[nodiscard]] inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(username_); }

    /**
     * @brief Uploads a file to the server.
     * @pre Assumes that \p path is a valid file.
     */
    bool Upload(std::filesystem::path&& path);

    inline void Flush() { payload_stream_.flush(); }

    [[nodiscard]] const std::string& GetUsername() const noexcept { return username_; }

   private:
    std::string username_;  ///< User's name, used as an identifier.

    Socket payload_socket_;  ///< Socket to exchange files.
    Socket client_sync;  ///< Socket for sync client -> server.
    Socket server_sync;  ///< Socket for sync server -> client.

    SocketStream payload_stream_;
    SocketStream client_stream_;
    SocketStream server_stream_;

    FileExchange   payload_fe_;  ///< What to exchange files with the server with.
    FileExchange   client_fe_;
    FileExchange   server_fe_;

    Inotify inotify_;

    bool client_sync_;
};

}
