#pragma once

#include <netinet/in.h>

#include <cstdint>
#include <filesystem>
#include <string>

#include "communication/protocol.hpp"
#include "communication/exchange_aggregate.hpp"

namespace dropbox {
class Client : public ExchangeAggregate {
   public:
    Client(std::string &&user_name, const char *server_ip_address, in_port_t port);

   private:
    std::string user_name_;
    int         server_socket_;
};
}
