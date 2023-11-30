#include "server.hpp"

#include <functional>
#include <iostream>
#include <thread>

#include "client_handler.hpp"
#include "connections.hpp"
#include "constants.hpp"
#include "exceptions.hpp"

dropbox::Server::Server(in_port_t port) : kReceiverSocket(socket(kDomain, kType, 0)), client_pool_() {
    if (kReceiverSocket == kInvalidSocket) {
        throw SocketCreation();
    }

    const sockaddr_in kReceiverAddress = {kFamily, htons(port), INADDR_ANY};

    if (bind(kReceiverSocket, reinterpret_cast<const sockaddr*>(&kReceiverAddress), sizeof(kReceiverAddress)) == -1) {
        throw Binding();
    }

    if (listen(kReceiverSocket, kBacklog) == -1) {
        throw Listening();
    }
}

[[noreturn]] void dropbox::Server::MainLoop() {
    sockaddr_in client_address{};
    socklen_t   client_length = sizeof(client_address);

    while (true) {
        const int kHeaderSocket = accept(kReceiverSocket, reinterpret_cast<sockaddr*>(&client_address), &client_length);
        const int kFileSocket   = accept(kReceiverSocket, reinterpret_cast<sockaddr*>(&client_address), &client_length);

        if (kHeaderSocket == kInvalidSocket) {
            if (kFileSocket == kInvalidSocket) {
                close(kHeaderSocket);
            }

            std::cerr << "Could not accept new client connection.\n";
            continue;
        }

        std::thread new_client_thread(
            [](int header_socket, int file_socket, ClientPool& pool) {
                try {
                    pool.Insert(ClientHandler(header_socket, file_socket)).MainLoop();
                } catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;  // NOLINT
                    close(header_socket);
                    close(file_socket);
                }
            },
            kHeaderSocket, kFileSocket, std::ref(client_pool_));

        new_client_thread.detach();
    }
}

dropbox::Server::~Server() { close(kReceiverSocket); }
