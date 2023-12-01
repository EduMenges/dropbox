#pragma once

#include <netinet/in.h>

#include <communication/protocol.hpp>
#include <cstdint>
#include <filesystem>
#include <string>

#include "utils.hpp"

namespace dropbox {

class Client {
   public:
    Client(std::string&& user_name, const char* server_ip_address, in_port_t port);

    /// Clients are not copiable due to side effect in socket closing.
    Client(const Client& other) = delete;

    Client(Client&& other) = default;

    ~Client();

    bool SendUsername();

    bool GetSyncDir();

    bool ListClient();

    bool ListServer();

    bool Download(std::filesystem::path&& file_name);

    bool Delete(std::filesystem::path&& file_name);

    bool Exit();

    bool ReceiveSyncFromServer();

    inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(username_); }

    /**
     * @brief Uploads a file to the server.
     * @pre Assumes that \p path is a valid file.
     */
    bool Upload(std::filesystem::path&& path);

   private:
    std::string username_;       ///< User's name, used as an identifier.
    int         header_socket_;  ///< Socket to exchange headers.
    int         file_socket_;    ///< Socket to exchange files.

    int         sync_socket_;    ///< Socket only for sync

    HeaderExchange    he_;
    FileExchange      fe_;
    DirectoryExchange de_;

    HeaderExchange she_;
    FileExchange   sfe_;
};

}
