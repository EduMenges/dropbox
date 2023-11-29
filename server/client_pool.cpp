#include "client_pool.hpp"

#include <iostream>

void dropbox::ClientPool::Insert(ClientHandler&& handler) {
    const int id       = handler.GetId();
    auto      username = handler.GetUsername();

    try {
        clients_[handler.GetUsername()].Insert(std::move(handler));
        std::cout << username << " connected with ID " << id << std::endl;  // NOLINT
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;  // NOLINT
    }
}
