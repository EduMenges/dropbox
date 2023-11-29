#include "client_composition.hpp"

bool dropbox::ClientAggregate::Insert(ClientHandler&& client) {
    std::lock_guard(mutex_);

    if (mutex_.size() <= kClientLimit) {
        list_.insert(std::move(client));
        return true;
    }

    return false;
}

void dropbox::ClientAggregate::Remove(int id) {
    std::lock_guard(mutex_);

    auto cu = std::find(list_.begin(), list_.end(), id);
}
