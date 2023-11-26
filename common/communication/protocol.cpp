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
    auto bytes_read = read(socket_, &this->command_, sizeof(this->command_));
    std::cerr << bytes_read << '\n';
    return bytes_read == sizeof(this->command_);
}

dropbox::Command dropbox::HeaderExchange::GetCommand() { return this->command_; }

bool dropbox::FileExchange::Send() {
    /// @todo Transferência de permissões

    std::basic_ifstream<char> file(path_, std::ios::in | std::ios::binary);

    if (!file.is_open() || file.bad()) {
        return false;
    }

    // Sending size
    auto file_size = std::filesystem::file_size(path_);
    write(socket_, &file_size, sizeof(file_size));

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
    }

    std::basic_ofstream<char> file(path_, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    // Receiving file size
    uintmax_t remaining_size = 0;
    read(socket_, &remaining_size, sizeof(remaining_size));

    while (remaining_size != 0) {
        const auto kBytesReceived = read(socket_, buffer.data(), kPacketSize);

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

    const auto kBytesReceived = read(socket_, received_path.data(), received_path.size());
    if (kBytesReceived == kInvalidRead) {
        perror(__func__);
        return false;
    }

    path_ = received_path.data();

    return true;
}

bool dropbox::EntryExchange::SendPath(const std::filesystem::path& path) const {
    auto path_len = strlen(path.c_str()) + 1;

    const auto kBytesSent = write(socket_, path.c_str(), path_len);

    if (kBytesSent == -1) {
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
        default:
            os.setstate(std::ios_base::failbit);
    }
    return os;
}
