#pragma once

#include <iostream>

#include "communication/protocol.hpp"

namespace dropbox {
class ClientHandler {
   public:
    ClientHandler(int socket_descriptor);
    ~ClientHandler();

    void MainLoop();

    void CreateUserFolder();

    bool ReceiveUsername();

    /// RECEIVES an upload from the client.
    bool ReceiveUpload();
    bool ReceiveDownload();
    bool ReceiveDelete();
    bool ReceiveGetSyncDir();

   private:
    int         socket_;
    std::string username_;

    HeaderExchange    he_;
    FileExchange      fe_;
    DirectoryExchange de_;

    bool sync_;
};
}
