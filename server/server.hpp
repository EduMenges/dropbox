#pragma once

#include <netinet/in.h>

#include "client_pool.hpp"
#include "networking/socket.hpp"
#include <csignal>

namespace dropbox {

class Server {
   public:
    /**
     * Constructor.
     * @param port Port to listen to new client connections to.
     */
    Server(in_port_t port);

    /// Server is not copiable due to side effect in destructor.
    Server(const Server& other) = delete;

    Server(Server&& other) = delete;

    ~Server() = default;

    /// Keep accepting new client connections in this loop.
    void MainLoop(sig_atomic_t& should_stop);

    /**
     * Builds a new client instance, inserts it into the pool, and starts its loop.
     * @param header_socket Header socket of the new client.
     * @param payload_socket File socket of the new client.
     */
    void NewClient(Socket&& header_socket, Socket&& payload_socket, Socket&& sync_sc_socket, Socket&& sync_cs_socket);

   private:
    static constexpr int kBacklog = 10; ///< Backlog in connection.

    Socket receiver_socket_; ///< Socket to listen to new connections to.
    ClientPool client_pool_; ///< Pool that stores information for all of the clients.
};

}
