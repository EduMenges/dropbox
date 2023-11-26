#include "server.hpp"

#include <iostream>
#include <thread>

#include "client_handler.hpp"
#include "connections.hpp"
#include "constants.hpp"
#include "exceptions.hpp"

dropbox::Server::Server(in_port_t port) : kReceiverSocket(socket(kDomain, kType, 0)) {
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

[[noreturn]] void dropbox::Server::MainLoop() const {
    sockaddr_in client_address{};
    socklen_t   client_length = sizeof(client_address);

    while (true) {
        const int kNewClient = accept(kReceiverSocket, reinterpret_cast<sockaddr*>(&client_address), &client_length);

        if (kNewClient == kInvalidSocket) {
            std::cerr << "Could not accept new client connection.\n";
        } else {
            std::thread new_client_thread([](auto socket_descriptor) { ClientHandler(socket_descriptor).MainLoop(); },
                                          kNewClient);
            new_client_thread.detach();
        }
    }
}

dropbox::Server::~Server() { close(kReceiverSocket); }
