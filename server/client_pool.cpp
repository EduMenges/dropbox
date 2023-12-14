#include "client_pool.hpp"

#include <iostream>

dropbox::ClientHandler& dropbox::ClientPool::Insert(dropbox::ClientHandler&& handler) noexcept(false) {
    const std::lock_guard kLockGuard(mutex_);
    return clients_[handler.GetUsername()].Insert(std::move(handler));
}
