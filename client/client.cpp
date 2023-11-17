#include "client.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <iostream>

#include "connections.hpp"
#include "exceptions.hpp"

dropbox::Client::Client(std::string &&user_name, const char *server_ip_address,
                        in_port_t port)
    : user_name_(std::move(user_name)),
      server_socket_(socket(kDomain, kType, kProtocol)) {
    if (this->server_socket_ == -1) {
        throw SocketCreation();
    }

    const sockaddr_in kServerAddress = {kFamily, htons(port),
                                        inet_addr(server_ip_address)};

    if (connect(server_socket_,
                reinterpret_cast<const sockaddr *>(&kServerAddress),
                sizeof(kServerAddress)) == -1) {
        throw Connecting();
    }
}
