#include "client_pool.hpp"

#include <iostream>

dropbox::ClientHandler& dropbox::ClientPool::Insert(dropbox::ClientHandler&& handler) noexcept(false) {
    handler.SetComposite(&clients_[handler.GetUsername()]);
    return clients_[handler.GetUsername()].Insert(std::move(handler));
}
