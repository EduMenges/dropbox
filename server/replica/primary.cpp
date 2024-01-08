#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "fmt/core.h"
#include "primary.hpp"

dropbox::replica::Primary::Primary(const std::string& ip) {
    const sockaddr_in kClientReceiverAddr = {kFamily, htons(kClientPort), {inet_addr(ip.c_str())}, {0}};
    const sockaddr_in kBackupAddr         = {kFamily, htons(kBackupPort), {inet_addr(ip.c_str())}, {0}};

    if (!client_receiver_.Bind(kClientReceiverAddr) || !backup_receiver_.Bind(kBackupAddr)) {
        throw Binding();
    }

    if (!client_receiver_.Listen(kBacklog) || !backup_receiver_.Listen(kBacklog)) {
        throw Listening();
    }

    //    client_receiver_.SetTimeout(kTimeout);
}

bool dropbox::replica::Primary::AcceptBackup() {
    Socket new_backup(accept(backup_receiver_, nullptr, nullptr));

    if (!new_backup.IsValid()) {
        return false;
    }

    fmt::println("{}: new server", __func__);
    backups_.emplace_back(std::move(new_backup));
    return true;
}

void dropbox::replica::Primary::AcceptBackupLoop() {
    accept_thread_ = std::jthread([&](const std::stop_token& stop_token) {
        while (!stop_token.stop_requested()) {
            AcceptBackup();
        }
    });
}

void dropbox::replica::Primary::MainLoop(sig_atomic_t& should_stop) {
    while (should_stop != 1) {
        Socket payload_socket(accept(client_receiver_, nullptr, nullptr));

        if (payload_socket == kInvalidSocket && errno == EAGAIN) {
            continue;
        }

        Socket client_sync(accept(client_receiver_, nullptr, nullptr));
        Socket server_sync(accept(client_receiver_, nullptr, nullptr));

        // Immediately stops the client building if any sockets are invalid.
        if (InvalidSockets(payload_socket, server_sync, client_sync)) {
            const auto kCurrentErrno = std::make_error_code(static_cast<std::errc>(errno));
            fmt::println(
                stderr, "MainLoop: could not accept new client connection {{ errno: {} }}", kCurrentErrno.message());
        } else {
            NewClient(std::move(payload_socket), std::move(client_sync), std::move(server_sync));
        }
    }
}

void dropbox::replica::Primary::NewClient(dropbox::Socket&& payload_socket, dropbox::Socket&& client_sync,
                                          dropbox::Socket&& server_sync) {
    auto thread_function = [this](
                               Socket&& payload_socket, Socket&& client_sync, Socket&& server_sync, ClientPool& pool) {
        try {
            SocketStream                       payload_stream(payload_socket);
            cereal::PortableBinaryInputArchive archive(payload_stream);
            std::string                        username;
            archive(username);

            ClientHandler& handler = pool.Emplace(std::move(username),
                                                  backups_,
                                                  std::move(payload_socket),
                                                  std::move(client_sync),
                                                  std::move(server_sync),
                                                  std::move(payload_stream));

            std::jthread const sync_thread([&](std::stop_token stop_token) { handler.SyncFromClient(stop_token); });

            handler.MainLoop();

            handler.GetComposite()->Remove(handler.GetId());
        } catch (std::exception& e) {
            fmt::println(stderr, "NewClient: {}", e.what());
        }
    };

    auto available_thread =
        std::ranges::find_if_not(client_threads_, [](std::jthread& thread) { return thread.joinable(); });

    if (available_thread == std::end(client_threads_)) {
        client_threads_.emplace_back(thread_function,
                                     std::move(payload_socket),
                                     std::move(client_sync),
                                     std::move(server_sync),
                                     std::ref(client_pool_));
    } else {
        *available_thread = std::jthread(thread_function,
                                         std::move(payload_socket),
                                         std::move(client_sync),
                                         std::move(server_sync),
                                         std::ref(client_pool_));
    }
}
