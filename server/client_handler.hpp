#pragma once

#include <iostream>

#include "communication/protocol.hpp"
#include "utils.hpp"

namespace dropbox {
class ClientHandler {
   public:
    ClientHandler(int socket_descriptor);

    /// Clients handlers are not copiable due to side effect in socket closing.
    ClientHandler(const ClientHandler& other) = delete;

    ClientHandler(ClientHandler&& other) = default;

    ~ClientHandler();

    void MainLoop();

    bool ReceiveUsername();

    /// RECEIVES an upload from the client.
    bool ReceiveUpload();

    bool ListServer();

    inline bool operator==(const ClientHandler& other) const noexcept {
        return socket_ == other.socket_;
    }

    inline bool operator==(int socket) const noexcept {
        return socket == socket_;
    }

    inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(username_); }

   private:
    int         socket_;
    std::string username_;

    HeaderExchange    he_;
    FileExchange      fe_;
    DirectoryExchange de_;
};
}
