#include "client_handler.hpp"

#include <dirent.h>
#include <unistd.h>

#include <filesystem>
#include <thread>
#include <vector>

#include "../common/utils.hpp"
#include "exceptions.hpp"
#include "inotify.hpp"

dropbox::ClientHandler::ClientHandler(int header_socket, int file_socket)
    : header_socket_(header_socket),
      file_socket_(file_socket),
      he_(header_socket),
      fe_(file_socket),
      de_(file_socket),
      sync_(false) {
    if (!ReceiveUsername()) {
        throw Username();
    }

    CreateUserFolder();

    std::cout << "NEW CLIENT: " << username_ << '\n';
}

bool dropbox::ClientHandler::ReceiveUsername() {
    static thread_local std::array<char, NAME_MAX + 1> buffer{};

    if (!he_.Receive() || he_.GetCommand() != Command::USERNAME) {
        return false;
    }

    size_t username_length = 0;
    if (read(file_socket_, &username_length, sizeof(username_length)) != sizeof(username_length)) {
        perror(__func__);
        return false;
    }

    const auto kBytesReceived = read(file_socket_, buffer.data(), username_length);

    if (kBytesReceived != username_length) {
        perror(__func__);  // NOLINT
        return false;
    }

    username_ = std::string(buffer.data());
    return true;
}

void dropbox::ClientHandler::MainLoop() {
    bool receiving = true;

    while (receiving) {
        if (he_.Receive()) {
            std::cout << username_ << " ordered " << he_.GetCommand() << std::endl;  // NOLINT
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

    const std::filesystem::path& file_path = fe_.GetPath();

    if (!std::filesystem::exists(file_path)) {
        std::cerr << "Error: File does not exist - " << file_path << '\n';

        return he_.SetCommand(Command::ERROR).Send();
    }

    const bool kCouldSendSuccess = he_.SetCommand(Command::SUCCESS).Send();
    if (!kCouldSendSuccess) {
        return false;
    }

    return fe_.Send();
}

bool dropbox::ClientHandler::ReceiveGetSyncDir() {
    std::filesystem::path              path(".");
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
    try {
        if (!std::filesystem::exists(SyncDirWithPrefix(username_))) {
            std::filesystem::create_directory(SyncDirWithPrefix(username_));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory " << e.what() << '\n';
    }
}

dropbox::ClientHandler::~ClientHandler() {
    std::cout << username_ << " disconnected" << std::endl;  // NOLINT
    close(header_socket_);
    close(file_socket_);
}
