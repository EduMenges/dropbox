#pragma once

#include <netinet/in.h>

#include "client_pool.hpp"

namespace dropbox {

class Server {
   public:
    Server(in_port_t port);

    /// Server is not copiable due to side effect in destructor.
    Server(const Server& other) = delete;

    Server(Server&& other) = default;

    ~Server();

    [[noreturn]] void MainLoop();

    void NewClient(int header_socket, int file_socket);

   private:
    static constexpr int kBacklog = 10;

    ClientPool client_pool_;

    const int kReceiverSocket;  // NOLINT
};

}
