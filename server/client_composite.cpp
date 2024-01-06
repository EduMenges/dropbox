#include "client_composite.hpp"

#include "exceptions.hpp"

void dropbox::ClientComposite::Remove(int id) {
    const std::lock_guard kLock(mutex_);

    list_.remove_if([id](auto& cli) { return cli == id; });
}

dropbox::ClientHandler& dropbox::ClientComposite::Emplace(Socket&& header_socket, SocketStream&& payload_stream,
                                                          Socket&& sync_sc_socket,
                                                          Socket&& sync_cs_socket) noexcept(false) {
    const std::lock_guard kLock(mutex_);

    if (list_.size() >= kDeviceLimit) {
        std::cerr << "Can't connect " << username_ << '\n';
        throw FullList();
    }

    auto& client = list_.emplace_back(this,
                                      std::move(header_socket),
                                      std::move(payload_stream),
                                      std::move(sync_sc_socket),
                                      std::move(sync_cs_socket));
    std::cout << "New client: " << client.GetUsername() << "::" << client.GetId() << '\n';
    return list_.back();
}

bool dropbox::ClientComposite::BroadcastCommand(
    const std::function<bool(ClientHandler&, const std::filesystem::path&)>& method, ClientHandler::IdType origin,
    const std::filesystem::path& path) {
    static std::atomic_uint8_t users = 0;

    users += 1;
    if (users == 1) {
        mutex_.lock();
    }

    for (auto& client : std::ranges::filter_view(list_, [&](const auto& i) { return i.GetId() != origin; })) {
        if (!method(client, path)) {
            return false;
        }
    }

    users -= 1;
    if (users == 0) {
        mutex_.unlock();
    }

    return true;
}
