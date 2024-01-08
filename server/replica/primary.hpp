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
     * AcceptBackup a new backup replica.
     * @return \c true if a backup connect, \c false if error or timeout.
     */
    bool AcceptBackup();

    void AcceptBackupLoop();

    /// Keep accepting new client connections in this loop.
    void MainLoop(std::atomic_bool& shutdown);

    /**
     * Builds a new client instance, inserts it into the pool, and starts its loop.
     * @param header_socket Header socket of the new client.
     * @param payload_socket File socket of the new client.
     */
    void NewClient(dropbox::Socket&& payload_socket, dropbox::Socket&& client_sync, dropbox::Socket&& server_sync);

    static constexpr in_port_t kClientPort = 12345;
    static constexpr in_port_t kBackupPort = 54321;

   private:
    static constexpr int     kBacklog = 10;  ///< Backlog in connection.
    static constexpr timeval kTimeout{2, 0};

    Socket client_receiver_;
    Socket backup_receiver_;

    std::jthread accept_thread_;
    std::vector<std::jthread> client_threads_;

    std::vector<BackupHandler> backups_;
    ClientPool                 client_pool_;  ///< Pool that stores information for all of the clients.
};
}
