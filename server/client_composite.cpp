#include "client_composite.hpp"

#include "exceptions.hpp"

dropbox::ClientHandler& dropbox::ClientComposite::Insert(ClientHandler&& client) {
    const std::lock_guard kLock(mutex_);

    if (list_.size() >= kDeviceLimit) {
        std::cerr << "Can't connect " << client.GetUsername() << ' ' << client.GetId() << std::endl; //NOLINT
        throw FullList();
    }

    std::cout << "New client: " << client.GetUsername() << " with id " << client.GetId() << '\n';
    list_.push_back(std::move(client));
    return list_.back();
}

void dropbox::ClientComposite::Remove(int id) {
    const std::lock_guard kLock(mutex_);

    list_.remove_if([id](auto& cli) { return cli == id; });
}
