#pragma once

#include <netinet/in.h>

#include <communication/protocol.hpp>
#include <cstdint>
#include <filesystem>
#include <string>

namespace dropbox {

class Client {
   public:
    Client(std::string&& user_name, const char* server_ip_address, in_port_t port);

    /// Clients are not copiable due to side effect in socket closing.
    Client(const Client& other) = delete;

    Client(Client&& other) = default;

    ~Client();

    [[nodiscard]] int GetSocket() const;

    bool SendUsername();

    bool GetSyncDir();

    bool Download(std::filesystem::path&& file_name);
    bool Delete(std::filesystem::path&& file_name);

    bool Exit();

    /**
     * @brief Uploads a file to the server.
     * @pre Assumes that \p path is a valid file.
     */
    bool Upload(std::filesystem::path&& path);

   private:
    std::string username_; ///< User's name, used as an identifier.
    int         server_socket_; ///< Socket to communicate with the server.

    HeaderExchange    he_;
    FileExchange      fe_;
    DirectoryExchange de_;
};

}
