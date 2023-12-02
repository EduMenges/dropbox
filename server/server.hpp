#pragma once

#include <netinet/in.h>

#include "client_pool.hpp"

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

    Server(Server&& other) = default;

    ~Server();

    /// Keep accepting new client connections in this loop.
    [[noreturn]] void MainLoop();

    /**
     * Builds a new client instance, inserts it into the pool, and starts its loop.
     * @param header_socket Header socket of the new client.
     * @param file_socket File socket of the new client.
     */
    void NewClient(int header_socket, int file_socket, int sync_sc_socket, int sync_cs_socket);

   private:
    static constexpr int kBacklog = 10; ///< Backlog in connection.

    int receiver_socket_; ///< Socket to listen to new connections to.
    ClientPool client_pool_; ///< Pool that stores informations for all of the clients.
};

}
