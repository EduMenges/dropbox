#include "client_handler.hpp"

#include <dirent.h>
#include <unistd.h>

#include <filesystem>
#include <thread>
#include <vector>

#include "../common/utils.hpp"
#include "exceptions.hpp"
#include "inotify.hpp"
#include "list_directory.hpp"


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
                    //if (!sync_) {
                        if (ReceiveGetSyncDir()) {
                            std::cout << "Starting to listen sync_dir" << '\n';
                            sync_ = true;
                            /*std::thread new_dir_thread(
                                [](auto username) {
                                    Inotify(username).Start();
                                }, username_);
                            new_dir_thread.detach();*/
                        }
                    //}
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
    const std::filesystem::path        sync_path = SyncDirWithPrefix(username_);
    std::vector<std::filesystem::path> file_names;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(sync_path)) {
            if (std::filesystem::is_regular_file(entry.path())) {
                file_names.push_back(entry.path().filename());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return false;
    }

    for (const auto& file_name : file_names) {
        std::cout << file_name << '\n';
        if (!he_.SetCommand(Command::SUCCESS).Send()) {
            return false;
        }

        if (!fe_.SetPath(sync_path / file_name.filename()).SendPath()) {
            return false;
        }

        if (!fe_.Send()) {
            return false;
        }
    }

    return he_.SetCommand(Command::EXIT).Send();
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

bool dropbox::ClientHandler::ListServer() {
    std::string str_table = ListDirectory(SyncDirPath()).str();

    const size_t kTableSize = str_table.size() + 1;

    if (write(header_socket_, &kTableSize, sizeof(kTableSize)) == kInvalidWrite) {
        perror(__func__);
        return false;
    }

    size_t total_sent = 0;
    while (total_sent != kTableSize) {
        const size_t kBytesToSend = std::min(kPacketSize, kTableSize - total_sent);

        if (write(header_socket_, str_table.data() + total_sent, kBytesToSend) == kInvalidWrite) {
            perror(__func__);
            return false;
        }

        total_sent += kTableSize;
    }

    return true;
}
