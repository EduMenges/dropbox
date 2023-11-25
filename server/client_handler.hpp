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
            FileExchange fe(socket_desc_);
            if (he.Receive()) {
                switch (he.GetCommand()) { 
                    case Command::UPLOAD:
                    case Command::DOWNLOAD:
                    case Command::DELETE:
                        if (fe.ReceivePath()) {
                            std::cout << "comando + path" << '\n';
                        }
                        break;
                    
                    default:
                        std::cout << "comando" << '\n';
                        break;
                }
            }
        }
    }

   private:
    int socket_desc_;
};
}
