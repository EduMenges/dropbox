#pragma once

#include <netinet/in.h>

namespace dropbox {

class Server {
   public:
    Server(in_port_t port);
    ~Server();

    [[noreturn]] void MainLoop() const;

   private:
    static constexpr int kBacklog = 10;

    const int kReceiverSocket;  // NOLINT
};

}
