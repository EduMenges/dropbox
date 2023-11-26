#include "client_handler.hpp"
#include "../common/constants.hpp"
#include "inotify.hpp"

#include <unistd.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <dirent.h>
#include <fstream>

#include <climits>

#include "exceptions.hpp"
#include "../common/utils.hpp"

dropbox::ClientHandler::ClientHandler(int socket_descriptor)
    : socket_(socket_descriptor),
      fe_(socket_descriptor),
      de_(socket_descriptor),
      he_(socket_descriptor),
      sync_(false),
      username_(NAME_MAX, '\0') {
    if (!ReceiveUsername()) {
        throw Username();
    }

    CreateUserFolder();

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
                    ReceiveDownload();
                    break;
                case Command::DELETE:
                    ReceiveDelete();
                    break;
                case Command::EXIT:
                    receiving = false;
                    break;
                case Command::GET_SYNC_DIR:
                    if (!sync_) {
                        if (ReceiveGetSyncDir()) {
                            std::cout << "Starting to listen sync_dir" << '\n';
                            sync_ = true;
                            std::thread new_dir_thread([]() { Inotify().Start(); });
                            new_dir_thread.detach();
                        }
                    }
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

bool dropbox::ClientHandler::ReceiveDownload() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    return fe_.Send();
}

bool dropbox::ClientHandler::ReceiveGetSyncDir() {
    std::string path = ".";
    std::vector<std::filesystem::path> file_names;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (std::filesystem::is_regular_file(entry.path())) {
                file_names.push_back(entry.path().filename());
            }
        }

        for (const auto& file_name : file_names) {
            std::cout << file_name << '\n';
            
            if (!fe_.SetPath(file_name.filename()).SendPath()) {
                return false;
            }
            if (!fe_.Send()) {
                return false;
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return false;
    }

    return true;
}

void dropbox::ClientHandler::CreateUserFolder() {
    try{
        if(!std::filesystem::exists(SyncDirWithPrefix(username_))) {
            std::filesystem::create_directory(SyncDirWithPrefix(username_));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory " << e.what() << '\n';
    }
}

dropbox::ClientHandler::~ClientHandler() {
    std::cerr << username_ << " disconnected\n";
    close(socket_);
}

