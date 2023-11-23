#pragma once

#include <iostream>

#include "../common/communication/protocol.hpp"

namespace dropbox {
class ClientHandler {
   public:
    ClientHandler(int socket_descriptor) : socket_desc_(socket_descriptor) {}

    void MainLoop() {
        std::cout << "Comecei com o soquete " << socket_desc_ << '\n';

        // Só teste
        while (true) {
            HeaderExchange he(socket_desc_);
            if (he.Receive()) {
                 std::cout << "Command sent successfully.\n";   
            }

            FileExchange fe(socket_desc_);
            if (fe.ReceivePath()) {
                std::cout << "Path sent successfully.\n";
            }
        }
    }

   private:
    int socket_desc_;
};
}
