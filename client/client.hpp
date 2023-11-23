#pragma once

#include <netinet/in.h>

#include <cstdint>
#include <string>

namespace dropbox {
class Client {
   public:
    Client(std::string &&user_name, const char *server_ip_address,
           in_port_t port);

    int GetSocket();

   private:
    std::string user_name_;
    int         server_socket_;
};
}
