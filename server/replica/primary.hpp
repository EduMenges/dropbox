#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>

#include <csignal>
#include <thread>
#include <vector>

#include "connections.hpp"
#include "BackupHandler.hpp"
#include "ClientPool.hpp"
#include "networking/socket.hpp"


namespace dropbox::replica {

class Primary {
   public:
    explicit Primary(const std::string& ip);

    Primary(Primary&& other) = delete;

    Primary(const Primary& other) = delete;

    ~Primary() = default;

    /**
     * Accept a new backup replica.
     * @return \c true if a backup connect, \c false if error or timeout.
     */
    bool Accept();

    void AcceptBackups();

    /// Keep accepting new client connections in this loop.
    void MainLoop(sig_atomic_t& should_stop);

    /**
     * Builds a new client instance, inserts it into the pool, and starts its loop.
     * @param header_socket Header socket of the new client.
     * @param payload_socket File socket of the new client.
     */
    void NewClient(Socket&& header_socket, Socket&& payload_socket, Socket&& sync_sc_socket, Socket&& sync_cs_socket);

    static constexpr in_port_t kAdminPort = 12345;

   private:
    static constexpr int     kBacklog = 10;  ///< Backlog in connection.
    static constexpr timeval kTimeout{1, 0};

    Socket       receiver_;
    std::jthread accept_thread_;

    std::vector<BackupHandler> backups_;
    ClientPool                 client_pool_;  ///< Pool that stores information for all of the clients.
};
}
