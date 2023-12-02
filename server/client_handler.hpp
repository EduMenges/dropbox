#pragma once

#include <iostream>

#include <thread>

#include "communication/protocol.hpp"
#include "utils.hpp"
#include "../common/inotify.hpp"

namespace dropbox {
class ClientHandler {
   public:
    ClientHandler(int header_socket, int file_socket, int sync_sc_socket, int sync_cs_socket);

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

    bool ReceiveSyncFromClient();

    inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(username_); }

   private:

    int         header_socket_;
    int         file_socket_;
    int         sync_sc_socket_;
    int         sync_cs_socket_;
    std::string username_;

    HeaderExchange    he_;
    FileExchange      fe_;
    DirectoryExchange de_;

    HeaderExchange    sche_;
    FileExchange      scfe_;
    HeaderExchange    cshe_;
    FileExchange      csfe_;

    Inotify inotify_;

};
}
