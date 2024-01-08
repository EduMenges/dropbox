#include "ClientComposite.hpp"

#include <atomic>
#include <ranges>

#include "exceptions.hpp"

void dropbox::ClientComposite::Remove(int id) {
    const std::lock_guard kLock(mutex_);

    list_.remove_if([id](auto& cli) { return cli == id; });
}

dropbox::ClientHandler& dropbox::ClientComposite::Emplace(Socket&& payload_socket, Socket&& client_sync,
                                                          Socket&&       server_sync,
                                                          SocketStream&& payload_stream) noexcept(false) {
    const std::lock_guard kLock(mutex_);

    if (list_.size() >= kDeviceLimit) {
        fmt::println(stderr, "ClientComposite::{}: can't connect new client {} due to full list", __func__, username_);
        throw FullList();
    }

    auto& client = list_.emplace_back(
        this, std::move(payload_socket), std::move(client_sync), std::move(server_sync), std::move(payload_stream));
    fmt::println("ClientComposite::{}: new client {}::{}", __func__, client.GetUsername(), client.GetId());
    return list_.back();
}

bool dropbox::ClientComposite::BroadcastCommand(
    const std::function<bool(ClientHandler&, const std::filesystem::path&)>& method,
    const std::function<bool(BackupHandler&, const std::filesystem::path&)>& backup_method,
    ClientHandler::IdType origin, const std::filesystem::path& path) {
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

    for (auto& backup : backups_) {
        if (!backup_method(backup, path)) {
            return false;
        }
    }

    users -= 1;
    if (users == 0) {
        mutex_.unlock();
    }

    return true;
}
