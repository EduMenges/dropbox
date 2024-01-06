#include "client_pool.hpp"

#include <iostream>

dropbox::ClientHandler& dropbox::ClientPool::Emplace(std::string&& username, Socket&& header_socket,
                                                     SocketStream&& payload_stream, Socket&& sync_sc_socket,
                                                     Socket&& sync_cs_socket) noexcept(false) {
    const std::lock_guard kLockGuard(mutex_);
    ClientComposite&      composite = clients_.try_emplace(username, std::move(username)).first->second;
    return composite.Emplace(
        std::move(header_socket), std::move(payload_stream), std::move(sync_sc_socket), std::move(sync_cs_socket));
}
