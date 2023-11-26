#pragma once

#include <iostream>

#include "communication/protocol.hpp"

namespace dropbox {
class ClientHandler {
   public:
    ClientHandler(int socket_descriptor);
    ~ClientHandler();

    void MainLoop();

    bool ReceiveUsername();

    /// RECEIVES an upload from the client.
    bool ReceiveUpload();

   private:
    int         socket_;
    std::string username_;

    HeaderExchange    he_;
    FileExchange      fe_;
    DirectoryExchange de_;
};
}
