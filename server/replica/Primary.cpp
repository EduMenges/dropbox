#include "Primary.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "fmt/core.h"

dropbox::replica::Primary::Primary(const std::string& ip, std::vector<std::string> clients_ips) {
    const sockaddr_in kClientReceiverAddr = {kFamily, htons(kClientPort), {inet_addr(ip.c_str())}, {0}};
    const sockaddr_in kBackupAddr         = {kFamily, htons(kBackupPort), {inet_addr(ip.c_str())}, {0}};

    client_receiver_.SetOpt(SOL_SOCKET, SO_REUSEPORT, 1);
    backup_receiver_.SetOpt(SOL_SOCKET, SO_REUSEPORT, 1);

    if (!client_receiver_.Bind(kClientReceiverAddr)) {
        fmt::println(stderr, "{}: when binding to client receiver", __func__);
        throw Binding();
    }

    if (!backup_receiver_.Bind(kBackupAddr)) {
        fmt::println(stderr, "{}: when binding to backup receiver", __func__);
        throw Binding();
    }

    if (!client_receiver_.Listen(kBacklog) || !backup_receiver_.Listen(kBacklog)) {
        throw Listening();
    }

    if (!backup_receiver_.SetTimeout(kTimeout)) {
        throw Socket::SetTimeoutException();
    }

    fmt::println("{}: constructed", __func__);

    if (!clients_ips.empty()) {
        for (const auto &client_ip : clients_ips) {
            fmt::println("trying: {}", client_ip);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            const sockaddr_in kClientAddress = {2, htons(6969), {inet_addr(client_ip.c_str())}, {0}};
            dropbox::Socket   payload_socket = dropbox::Socket();
            dropbox::Socket   client_sync    = dropbox::Socket();
            dropbox::Socket   server_sync    = dropbox::Socket();

            if (!payload_socket.Connect(kClientAddress) || !client_sync.Connect(kClientAddress) ||
                !server_sync.Connect(kClientAddress)) {
            } else {
                NewClient(std::move(payload_socket), std::move(client_sync), std::move(server_sync));
            }
        }
    }

}

void dropbox::replica::Primary::AcceptBackupLoop() {
    accept_thread_ = std::jthread([&](const std::stop_token& stop_token) {
        while (!stop_token.stop_requested()) {
            Socket new_backup(accept(backup_receiver_.Get(), nullptr, nullptr));

            if (!new_backup.IsValid()) {
                continue;
            }

            fmt::println("AcceptBackupLoop: new server {}", new_backup.Get());
            backups_.emplace_back(std::move(new_backup));
        }
    });
}

void dropbox::replica::Primary::MainLoop(std::atomic_bool& shutdown) {
    while (!shutdown) {
        struct sockaddr_in cli_addr;
        int cli_len = sizeof(cli_addr);

        Socket payload_socket(accept(client_receiver_.Get(), reinterpret_cast<struct sockaddr*>(&cli_addr),
                                     reinterpret_cast<socklen_t*>(&cli_len)));

        client_ip_ = inet_ntoa(cli_addr.sin_addr);

        if (!payload_socket.IsValid() && (errno == EAGAIN || errno == EINTR)) {
            continue;
        }

        Socket client_sync(accept(client_receiver_.Get(), nullptr, nullptr));
        Socket server_sync(accept(client_receiver_.Get(), nullptr, nullptr));

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
                                                  std::move(payload_stream),
                                                  client_ip_);

            handler.SetServers(GetServers());
            handler.GetComposite()->BroadcastIp(payload_socket.Get(), client_ip_);

            std::jthread const kSyncThread([&](std::stop_token stop_token) { handler.SyncFromClient(stop_token); });

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
