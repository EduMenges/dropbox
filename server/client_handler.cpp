#include "client_handler.hpp"

#include <unistd.h>

#include <climits>

#include "exceptions.hpp"
#include "list_directory.hpp"

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
                case Command::LIST_SERVER:
                    ListServer();
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

bool dropbox::ClientHandler::ListServer() {
    std::string str_table = ListDirectory(SyncDirPath()).str();

    const size_t kTableSize = str_table.size() + 1;

    if (write(socket_, &kTableSize, sizeof(kTableSize)) == kInvalidWrite) {
        perror(__func__);
        return false;
    }

    size_t total_sent = 0;
    while (total_sent != kTableSize) {
        const size_t kBytesToSend = std::min(kPacketSize, kTableSize - total_sent);

        if (write(socket_, str_table.data() + total_sent, kBytesToSend) == kInvalidWrite) {
            perror(__func__);
            return false;
        }

        total_sent += kTableSize;
    }

    return true;
}
