#include "primary.hpp"

dropbox::PrimaryReplica::PrimaryReplica(const std::string& ip) {
    sockaddr_in addr = {kFamily, htons(kAdminPort), {inet_addr(ip.c_str())}};

    if (bind(receiver_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == -1) {
        throw Binding();
    }

    if (listen(receiver_, 10) == -1) {
        throw Listening();
    }

    SetTimeout(receiver_, kTimeout);
}

bool dropbox::PrimaryReplica::Accept() {
    Socket new_backup(accept(receiver_, nullptr, nullptr));

    if (!new_backup.IsValid()) {
        return false;
    }

    fmt::println("New server");
    backups_.push_back(std::move(new_backup));
    return true;
}

void dropbox::PrimaryReplica::AcceptLoop() {
    accept_thread_ = std::jthread([&](const std::stop_token& stop_token) {
        while (!stop_token.stop_requested()) {
            Accept();
        }
    });
}
