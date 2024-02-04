#include "ClientPool.hpp"
#include <utility>

dropbox::ClientHandler& dropbox::ClientPool::Emplace(std::string&& username, std::vector<BackupHandler>& backups,
                                                     Socket&& payload_socket, Socket&& client_sync,
                                                     Socket&&       server_sync,
                                                     SocketStream&& payload_stream,
                                                     std::string client_ip) noexcept(false) {
    const std::lock_guard kLockGuard(mutex_);
    ClientComposite&      composite = clients_.try_emplace(username, std::move(username), backups).first->second;

    // HERE
    return composite.Emplace(
        std::move(payload_socket), std::move(client_sync), std::move(server_sync), std::move(payload_stream), client_ip);
}
