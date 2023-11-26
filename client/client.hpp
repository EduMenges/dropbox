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

    ~Client();

    [[nodiscard]] int GetSocket() const;

    bool SendUsername();

    bool GetSyncDir();

    bool Download(std::filesystem::path&& file_name);

    /// Assumes that \p path is a valid file.
    bool Upload(std::filesystem::path&& path);

   private:
    std::string username_;
    int         server_socket_;

    HeaderExchange    he_;
    FileExchange      fe_;
    DirectoryExchange de_;
};
}
