#include "protocol.hpp"

#include <unistd.h>

#include <climits>
#include <cstring>
#include <fstream>
#include <iostream>

#include "constants.hpp"

thread_local std::array<char, dropbox::FileExchange::kPacketSize> dropbox::FileExchange::buffer;

bool dropbox::HeaderExchange::Send() {
    auto bytes_sent = write(socket_, &this->command_, sizeof(this->command_));

    if (bytes_sent == kInvalidWrite) {
        perror(__func__);
    }

    return bytes_sent == sizeof(this->command_);
}

bool dropbox::HeaderExchange::Receive() {
    const ssize_t kBytesRead = read(socket_, &command_, sizeof(command_));

    if (kBytesRead == kInvalidRead) {
        perror(__func__);
        return false;
    }

    return kBytesRead == sizeof(command_);
}

bool dropbox::FileExchange::Send() {
    /// @todo Transferência de permissões

    std::basic_ifstream<char> file(path_, std::ios::in | std::ios::binary);

    if (!file.is_open() || file.bad()) {
        return false;
    }

    // Sending size
    uintmax_t file_size = std::filesystem::file_size(path_);
    if (write(socket_, &file_size, sizeof(file_size)) < sizeof(file_size)) {
        perror(__func__);
        return false;
    }

    do {
        const auto    kBytesRead = file.read(buffer.data(), kPacketSize).gcount();
        const ssize_t kBytesSent = write(socket_, buffer.data(), kBytesRead);

        if (kBytesSent < kBytesRead) {
            return false;
        }
    } while (!file.eof());

    return true;
}

bool dropbox::FileExchange::Receive() {
    if (!std::filesystem::is_regular_file(path_)) {
        std::filesystem::remove_all(path_);

        if (path_.has_parent_path()) {
            std::filesystem::create_directories(path_.parent_path());
        }
    }

    std::basic_ofstream<char> file(path_, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    // Receiving file size
    uintmax_t remaining_size = 0;

    if (read(socket_, &remaining_size, sizeof(remaining_size)) < sizeof(remaining_size)) {
        perror(__func__);
        return false;
    }

    while (remaining_size != 0) {
        const auto kBytesToReceive = std::min(remaining_size, kPacketSize);
        const auto kBytesReceived  = read(socket_, buffer.data(), kBytesToReceive);

        if (kBytesReceived == kInvalidRead) {
            return false;
        }

        file.write(buffer.data(), kBytesReceived);
        remaining_size -= kBytesReceived;
    }

    return true;
}

bool dropbox::EntryExchange::ReceivePath() {
    static std::array<char, PATH_MAX> received_path;

    size_t path_len = 0;
    read(socket_, &path_len, sizeof(path_len));

    const auto kBytesReceived = read(socket_, received_path.data(), path_len);

    if (kBytesReceived == kInvalidRead) {
        perror(__func__);
        return false;
    }

    path_ = received_path.data();

    return true;
}

bool dropbox::EntryExchange::SendPath(const std::filesystem::path& path) const {
    auto path_len = strlen(path.c_str()) + 1;

    write(socket_, &path_len, sizeof(path_len));

    const auto kBytesSent = write(socket_, path.c_str(), path_len);

    if (kBytesSent == kInvalidWrite) {
        perror(__func__);
        return false;
    }

    return kBytesSent == strlen(path.c_str()) + 1;
}

bool dropbox::DirectoryExchange::Send() {
    /// @todo This
    return false;
}
bool dropbox::DirectoryExchange::Receive() {
    /// @todo This
    return false;
}

std::ostream& dropbox::operator<<(std::ostream& os, dropbox::Command command) {
    switch (command) {
        case Command::UPLOAD:
            os << "upload";
            break;
        case Command::DELETE:
            os << "delete";
            break;
        case Command::GET_SYNC_DIR:
            os << "get_sync_dir";
            break;
        case Command::LIST_SERVER:
            os << "list_server";
            break;
        case Command::EXIT:
            os << "exit";
            break;
        case Command::DOWNLOAD:
            os << "download";
            break;
        default:
            break;
    }
    return os;
}
