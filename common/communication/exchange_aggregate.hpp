#pragma once

#include "communication/protocol.hpp"

namespace dropbox {
class ExchangeAggregate {
   public:
    ExchangeAggregate() = default;

    inline ExchangeAggregate(int socket_fd)
        : header_exchange_(socket_fd), file_exchange_(socket_fd), directory_exchange_(socket_fd){};

    bool Upload(std::filesystem::path&& path);

   protected:
    HeaderExchange    header_exchange_;
    FileExchange      file_exchange_;
    DirectoryExchange directory_exchange_;
};
}
