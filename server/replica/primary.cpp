#include "primary.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "fmt/core.h"

dropbox::replica::Primary::Primary(const std::string& ip) {
    sockaddr_in addr = {kFamily, htons(kAdminPort), {inet_addr(ip.c_str())}};

    if (bind(receiver_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == -1) {
        throw Binding();
    }

    if (listen(receiver_, kBacklog) == -1) {
        throw Listening();
    }

    SetTimeout(receiver_, kTimeout);
}

bool dropbox::replica::Primary::Accept() {
    Socket new_backup(accept(receiver_, nullptr, nullptr));

    if (!new_backup.IsValid()) {
        return false;
    }

    fmt::println("New server");
    backups_.emplace_back(std::move(new_backup));
    return true;
}

void dropbox::replica::Primary::AcceptBackups() {
    accept_thread_ = std::jthread([&](const std::stop_token& stop_token) {
        while (!stop_token.stop_requested()) {
            Accept();
        }
    });
}

void dropbox::replica::Primary::MainLoop(sig_atomic_t& should_stop) {
    while (should_stop != 1) {
        Socket header_socket(accept(receiver_, nullptr, nullptr));
        Socket payload_socket(accept(receiver_, nullptr, nullptr));
        Socket sync_sc_socket(accept(receiver_, nullptr, nullptr));
        Socket sync_cs_socket(accept(receiver_, nullptr, nullptr));

        if (header_socket.IsValid()) {
            NewClient(std::move(header_socket),
                      std::move(payload_socket),
                      std::move(sync_sc_socket),
                      std::move(sync_cs_socket));
        }
    }
}

void dropbox::replica::Primary::NewClient(dropbox::Socket&& header_socket, dropbox::Socket&& payload_socket,
                                        dropbox::Socket&& sync_sc_socket, dropbox::Socket&& sync_cs_socket) {
    std::thread new_client_thread(
        [this](Socket&&    header_socket,
               Socket&&    payload_socket,
               Socket&&    sync_sc_socket,
               Socket&&    sync_cs_socket,
               ClientPool& pool) {
            // Immediately stops the client building if any sockets are invalid.
            if (InvalidSockets(header_socket, payload_socket, sync_sc_socket, sync_cs_socket)) {
                fmt::println(stderr, "Could not accept new client connection due to invalid sockets");
                return;
            }

            try {
                SocketStream                       payload_stream(payload_socket);
                cereal::PortableBinaryInputArchive archive(payload_stream);
                std::string                        username;
                archive(username);

                ClientHandler& handler = pool.Emplace(std::move(username),
                                                      backups_,
                                                      std::move(header_socket),
                                                      std::move(payload_stream),
                                                      std::move(sync_sc_socket),
                                                      std::move(sync_cs_socket));

                std::thread sync_thread([&]() { handler.SyncFromClient(); });

                handler.MainLoop();

                handler.GetComposite()->Remove(handler.GetId());

                sync_thread.join();
            } catch (std::exception& e) {
                std::cerr << "Error when creating client: " << e.what() << '\n';
            }
        },
        std::move(header_socket),
        std::move(payload_socket),
        std::move(sync_sc_socket),
        std::move(sync_cs_socket),
        std::ref(client_pool_));

    new_client_thread.detach();
}
