#pragma once

#include <iostream>

#include <thread>

#include "communication/protocol.hpp"
#include "utils.hpp"

namespace dropbox {
class ClientHandler {
   public:
    ClientHandler(int header_socket, int file_socket);

    /// Clients handlers are not copiable due to side effect in socket closing.
    ClientHandler(const ClientHandler& other) = delete;

    ClientHandler(ClientHandler&& other) = default;

    ~ClientHandler();

    void MainLoop();

    void CreateUserFolder();

    bool ReceiveUsername();

    /// RECEIVES an upload from the client.
    bool ReceiveUpload();
    bool ReceiveDownload();
    bool ReceiveDelete();
    bool ReceiveGetSyncDir();

    bool ListServer();

    inline bool operator==(const ClientHandler& other) const noexcept {
        return socket_ == other.socket_;
    }

    inline bool operator==(int socket) const noexcept {
        return socket == socket_;
    }

    inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(username_); }

   private:

    int         header_socket_;
    int         file_socket_;
    std::string username_;

    HeaderExchange    he_;
    FileExchange      fe_;
    DirectoryExchange de_;

    std::thread inotify_server_thread;

    bool sync_;
};
}
