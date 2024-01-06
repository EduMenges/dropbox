#include "server.hpp"

#include <functional>
#include <iostream>
#include <thread>

#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "client_handler.hpp"
#include "connections.hpp"
#include "exceptions.hpp"
#include "fmt/core.h"

dropbox::Server::Server(in_port_t port) {
    const sockaddr_in kReceiverAddress = {kFamily, htons(port), {INADDR_ANY}};

    if (bind(receiver_socket_, reinterpret_cast<const sockaddr*>(&kReceiverAddress), sizeof(kReceiverAddress)) == -1) {
        throw Binding();
    }

    if (listen(receiver_socket_, kBacklog) == -1) {
        throw Listening();
    }
}

void dropbox::Server::MainLoop(sig_atomic_t& should_stop) {
    while (should_stop != 1) {
        Socket header_socket(accept(receiver_socket_, nullptr, nullptr));
        Socket payload_socket(accept(receiver_socket_, nullptr, nullptr));
        Socket sync_sc_socket(accept(receiver_socket_, nullptr, nullptr));
        Socket sync_cs_socket(accept(receiver_socket_, nullptr, nullptr));

        NewClient(
            std::move(header_socket), std::move(payload_socket), std::move(sync_sc_socket), std::move(sync_cs_socket));
    }
}

void dropbox::Server::NewClient(Socket&& header_socket, Socket&& payload_socket, Socket&& sync_sc_socket,
                                Socket&& sync_cs_socket) {
    std::thread new_client_thread(
        [](Socket&&    header_socket,
           Socket&&    payload_socket,
           Socket&&    sync_sc_socket,
           Socket&&    sync_cs_socket,
           ClientPool& pool) {
            // Immediately stops the client building if any sockets are invalid.
            if (InvalidSockets(header_socket, payload_socket, sync_sc_socket, sync_cs_socket)) {
                fmt::println(stderr, "Could not accept new client connection due to invalid sockets");
                perror(__func__);
                return;
            }

            try {
                SocketStream                       payload_stream(payload_socket);
                cereal::PortableBinaryInputArchive archive(payload_stream);
                std::string                        username;
                archive(username);

                ClientHandler& handler = pool.Emplace(std::move(username),
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
