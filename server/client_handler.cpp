#include "client_handler.hpp"

#include <unistd.h>

#include <climits>

#include "exceptions.hpp"

dropbox::ClientHandler::ClientHandler(int socket_descriptor)
    : socket_(socket_descriptor),
      fe_(socket_descriptor),
      de_(socket_descriptor),
      he_(socket_descriptor),
      username_(NAME_MAX, '\0') {
    if (!ReceiveUsername()) {
        throw Username();
    }

    std::cout << "NEW CLIENT: " << username_ << '\n';
}

bool dropbox::ClientHandler::ReceiveUsername() {
    const auto kBytesReceived = read(socket_, username_.data(), NAME_MAX);

    if (kBytesReceived == -1) {
        perror(__func__);  // NOLINT
        return false;
    }

    return true;
}

void dropbox::ClientHandler::MainLoop() {
    bool receiving = true;
    while (receiving) {
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
                case Command::EXIT:
                    receiving = false;
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

dropbox::ClientHandler::~ClientHandler() {
    std::cerr << username_ << " disconnected\n";
    close(socket_);
}
