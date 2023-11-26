#include "client_handler.hpp"

#include <unistd.h>

#include <climits>
#include <exceptions.hpp>

dropbox::ClientHandler::ClientHandler(int socket_descriptor)
    : socket_(socket_descriptor),
      fe_(socket_descriptor),
      de_(socket_descriptor),
      he_(socket_descriptor),
      username_(NAME_MAX + 1, '\0') {
    if (!ReceiveUsername()) {
        throw Username();
    }

    std::cout << "NEW CLIENT: " << username_ << '\n';
}

bool dropbox::ClientHandler::ReceiveUsername() {
    static thread_local std::array<char, NAME_MAX> buffer;

    const auto kBytesReceived = read(socket_, username_.data(), NAME_MAX);

    if (kBytesReceived == -1) {
        perror(__func__);  // NOLINT
        return false;
    }

    username_ = buffer.data();

    return true;
}

void dropbox::ClientHandler::MainLoop() {
    while (true) {
        if (he_.Receive()) {
            std::cout << username_ << " ordered " << he_.GetCommand() << '\n';
            switch (he_.GetCommand()) {
                case Command::UPLOAD:
                    ReceiveUpload();
                    break;
                case Command::DOWNLOAD:
                    break;
                case Command::DELETE:
                    break;
                default:
                    break;
            }
        }
    }
}

bool dropbox::ClientHandler::ReceiveUpload() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    return fe_.Receive();
}

dropbox::ClientHandler::~ClientHandler() { close(socket_); }
