#include "client_handler.hpp"
#include "../common/constants.hpp"

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

    CreateUserFolder();

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
                    ReceiveDownload();
                    break;
                case Command::DELETE:
                    ReceiveDelete();
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

bool dropbox::ClientHandler::ReceiveDownload() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    return fe_.Send();
}

bool dropbox::ClientHandler::ReceiveDelete() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    const std::filesystem::path& file_path = fe_.GetPath();
    
    if (std::filesystem::exists(file_path)) {
        std::filesystem::remove(file_path);
        return true;
    }

    return false;
}

void dropbox::ClientHandler::CreateUserFolder() {
    try{
        if(!std::filesystem::exists( kSyncDirPath + getSyncDir(username_.c_str()) )) {
            std::filesystem::create_directory(kSyncDirPath + getSyncDir(username_.c_str()));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory " << e.what() << '\n';
    }
}

dropbox::ClientHandler::~ClientHandler() { close(socket_); }
