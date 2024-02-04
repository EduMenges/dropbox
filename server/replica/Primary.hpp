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
#include "networking/Addr.hpp"

namespace dropbox::replica {

class Primary {
   public:
    explicit Primary(const std::string& ip, std::vector<std::string> clients_ips = {});

    Primary(Primary&& other) = delete;

    Primary(const Primary& other) = delete;

    ~Primary() = default;

    /// Creates a thread that concurrently runs until the destruction of @c this.
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

    void SetServers(const std::vector<Addr>& servers) {
        for (const auto& server : servers) {
            servers_ += server.GetIp() + " ";
        }
    }

    std::string GetServers() { return servers_; }

    void SetClientsIps(std::vector<std::string> ips) {
        list_client_ip_ = ips;
    }

   private:
    static constexpr int     kBacklog = 10;   ///< Backlog in connection.
    static constexpr timeval kTimeout{2, 0};  ///< Timeout used in @p backup_receiver_ to allow a graceful exit.

    Socket client_receiver_;  ///< Where to listen to receive new clients.
    Socket backup_receiver_;  ///< Where to listen to receive new backup replicas.

    std::jthread              accept_thread_;   ///< Thread to accept new backup replicas.
    std::vector<std::jthread> client_threads_;  ///< Threads of the clients.

    std::vector<BackupHandler> backups_;      ///< Backup replicas.
    ClientPool                 client_pool_;  ///< Pool that stores information for all of the clients.
    std::string                servers_;
    std::string                client_ip_;
    std::vector<std::string>   list_client_ip_;
};
}
