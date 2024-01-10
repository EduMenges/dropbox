#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>

#include <csignal>
#include <thread>
#include <vector>

#include "connections.hpp"
#include "BackupHandler.hpp"
#include "ClientPool.hpp"
#include "networking/Socket.hpp"

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

    /// Creates a thread that concurrently runs @ref dropbox::Primary::AcceptBackup until the destruction of @c this.
    void AcceptBackupLoop();

    /// Keep accepting new client connections in this loop.
    void MainLoop(std::atomic_bool& shutdown);

    /**
     * Inserts a new client into the pool.
     * @param payload_socket Socket to be used as the payload.
     * @param client_sync Socket used in client sync (inotify).
     * @param server_sync Socket used in server sync.
     */
    void NewClient(dropbox::Socket&& payload_socket, dropbox::Socket&& client_sync, dropbox::Socket&& server_sync);

    static constexpr in_port_t kClientPort = 12345;  ///< Where the clients are expected to connect to.
    static constexpr in_port_t kBackupPort = 54321;  ///< Where the backup replicas are expected to connect to.

   private:
    static constexpr int     kBacklog = 10;   ///< Backlog in connection.
    static constexpr timeval kTimeout{2, 0};  ///< Timeout used in @p backup_receiver_ to allow a graceful exit.

    Socket client_receiver_;  ///< Where to listen to receive new clients.
    Socket backup_receiver_;  ///< Where to listen to receive new backup replicas.

    std::jthread              accept_thread_;   ///< Thread to accept new backup replicas.
    std::vector<std::jthread> client_threads_;  ///< Threads of the clients.

    std::vector<BackupHandler> backups_;      ///< Backup replicas.
    ClientPool                 client_pool_;  ///< Pool that stores information for all of the clients.
};
}
