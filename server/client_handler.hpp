#pragma once

#include <filesystem>
#include <iostream>

#include "communication/protocol.hpp"
#include "communication/exchange_aggregate.hpp"

namespace dropbox {
class ClientHandler : public ExchangeAggregate {
   public:
    inline ClientHandler(int socket_fd)
        : socket_(socket_fd), ExchangeAggregate(socket_fd) {};

    void MainLoop();

    bool Upload(std::filesystem::path&& kPath);

   private:
    int socket_;
};
}
