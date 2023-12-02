#include "server.hpp"

#include <functional>
#include <iostream>
#include <thread>

#include "client_handler.hpp"
#include "connections.hpp"
#include "constants.hpp"
#include "exceptions.hpp"

dropbox::Server::Server(in_port_t port) : receiver_socket_(socket(kDomain, kType, 0)) {
    if (receiver_socket_ == kInvalidSocket) {
        throw SocketCreation();
    }

    const sockaddr_in kReceiverAddress = {kFamily, htons(port), {INADDR_ANY}};

    if (bind(receiver_socket_, reinterpret_cast<const sockaddr*>(&kReceiverAddress), sizeof(kReceiverAddress)) == -1) {
        throw Binding();
    }

    if (listen(receiver_socket_, kBacklog) == -1) {
        throw Listening();
    }
}

[[noreturn]] void dropbox::Server::MainLoop() {
    sockaddr_in client_address{};
    socklen_t   client_length = sizeof(client_address);

    while (true) {
/* old
        const int kHeaderSocket = accept(kReceiverSocket, reinterpret_cast<sockaddr*>(&client_address), &client_length);
        const int kFileSocket   = accept(kReceiverSocket, reinterpret_cast<sockaddr*>(&client_address), &client_length);
        const int kSyncSCSocket = accept(kReceiverSocket, reinterpret_cast<sockaddr*>(&client_address), &client_length);
        const int kSyncCSSocket = accept(kReceiverSocket, reinterpret_cast<sockaddr*>(&client_address), &client_length);

        if (kHeaderSocket == kInvalidSocket) {
            if (kFileSocket == kInvalidSocket) {
                close(kHeaderSocket);
            }
 
            std::cerr << "Could not accept new client connection.\n";
        } else {
            std::thread new_client_thread(
                [](int header_socket, int file_socket, int sync_sc_socket, int sync_cs_socket) {
                    try {
                        ClientHandler(header_socket, file_socket, sync_sc_socket, sync_cs_socket).MainLoop();
                    } catch (std::exception& e) {
                        std::cerr << e.what() << std::endl;  // NOLINT
                        close(header_socket);
                        close(file_socket);
                        close(sync_sc_socket);
                        close(sync_cs_socket);
                    }
                },
                kHeaderSocket, kFileSocket, kSyncSCSocket, kSyncCSSocket);

            new_client_thread.detach();
        }
*/
        const int kHeaderSocket = accept(receiver_socket_, reinterpret_cast<sockaddr*>(&client_address), &client_length);
        const int kFileSocket   = accept(receiver_socket_, reinterpret_cast<sockaddr*>(&client_address), &client_length);
        const int kSyncSCSocket = accept(receiver_socket_, reinterpret_cast<sockaddr*>(&client_address), &client_length);
        const int kSyncCSSocket = accept(receiver_socket_, reinterpret_cast<sockaddr*>(&client_address), &client_length);

        NewClient(kHeaderSocket, kFileSocket, kSyncSCSocket, kSyncCSSocket);

    }
}

dropbox::Server::~Server() { close(receiver_socket_); }

void dropbox::Server::NewClient(int header_socket, int file_socket, int sync_sc_socket, int sync_cs_socket) {
    std::thread new_client_thread(
        [](int header_socket, int file_socket, int sync_sc_socket, int sync_cs_socket, ClientPool& pool) {
            // Immediately stops the client building if any sockets are invalid.
            if (header_socket == kInvalidSocket) {
                if (file_socket == kInvalidSocket) {
                    close(header_socket);
                }
                std::cerr << "Could not accept new client connection\n";
                return;
            }

            try {
                ClientHandler new_handler(header_socket, file_socket, sync_sc_socket, sync_cs_socket);

                ClientHandler& handler = pool.Insert(std::move(new_handler));
                handler.MainLoop();
                handler.GetComposite()->Remove(handler.GetId());
            } catch (std::exception& e) {
                std::cerr << e.what() << std::endl;  // NOLINT
            }
        },
        header_socket, file_socket, sync_sc_socket, sync_cs_socket, std::ref(client_pool_));
    new_client_thread.detach();
}
