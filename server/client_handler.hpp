#pragma once

#include <iostream>

namespace dropbox {
class ClientHandler {
   public:
    ClientHandler(int socket_descriptor) : socket_desc_(socket_descriptor) {}

    void MainLoop() {
        std::cout << "Comecei com o soquete " << socket_desc_ << '\n';
    }

   private:
    int socket_desc_;
};
}
