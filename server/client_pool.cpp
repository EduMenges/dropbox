#include "client_pool.hpp"

#include <iostream>

dropbox::ClientHandler& dropbox::ClientPool::Emplace(std::string&& username, dropbox::SocketType header_socket,
                                                     dropbox::SocketStream payload_stream,
                                                     dropbox::SocketType sync_sc_socket,
                                                     dropbox::SocketType sync_cs_socket) noexcept(false) {
    const std::lock_guard kLockGuard(mutex_);
    ClientComposite& composite = clients_.try_emplace(username, std::move(username)).first->second;
    return composite.Emplace(header_socket, std::move(payload_stream), sync_sc_socket, sync_cs_socket);
}
