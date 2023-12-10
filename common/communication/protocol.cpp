#include "protocol.hpp"

#include <unistd.h>

#include <climits>
#include <cstring>
#include <fstream>
#include <iostream>

#include "utils.hpp"

thread_local std::array<char, dropbox::kPacketSize> dropbox::FileExchange::buffer;

bool dropbox::HeaderExchange::Send() {
    auto bytes_sent = write(socket_, &command_, sizeof(command_));

    if (bytes_sent != sizeof(command_)) {
        perror(__func__);
        return false;
    }

    return true;
}

bool dropbox::HeaderExchange::Receive() {
    const ssize_t kBytesRead = read(socket_, &command_, sizeof(command_));

    if (kBytesRead == kInvalidRead) {
        perror("HeaderExchange::Receive");
        return false;
    }

    if (kBytesRead < SSizeOf<Command>()) {
        std::cerr << __func__ << ": received " << kBytesRead << " bytes\n";
    }

    return true;
}

bool dropbox::FileExchange::Send() {
    std::basic_ifstream<char> file(path_, std::ios::in | std::ios::binary);

    if (!file.is_open() || file.bad()) {
        return false;
    }

    // Sending size
    uintmax_t file_size = std::filesystem::file_size(path_);
    if (write(socket_, &file_size, sizeof(file_size)) != SSizeOf(file_size)) {
        perror(__func__);
        return false;
    }

    do {
        const auto    kBytesRead = file.read(buffer.data(), kPacketSize).gcount();
        const ssize_t kBytesSent = write(socket_, buffer.data(), kBytesRead);

        if (kBytesSent != kBytesRead) {
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

    if (read(socket_, &remaining_size, sizeof(remaining_size)) < SSizeOf(remaining_size)) {
        perror("FileExchange::Receive");
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

bool dropbox::EntryExchange::SendPath(const std::filesystem::path& path) {
    size_ = strlen(path.c_str()) + 1;
    SendSize();

    const auto kBytesSent = write(socket_, path.c_str(), size_);

    if (kBytesSent != static_cast<ssize_t>(size_)) {
        perror(__func__);
        return false;
    }

    return true;
}

bool dropbox::EntryExchange::ReceivePath() {
    static thread_local std::array<char, PATH_MAX> received_path;

    ReceiveSize();
    const ssize_t kBytesReceived = read(socket_, received_path.data(), size_);

    if (kBytesReceived != static_cast<ssize_t>(size_)) {
        perror(__func__);
        return false;
    }

    path_ = received_path.data();

    return true;
}

bool dropbox::SocketExchange::SendSize() const {
    const auto kConvertedSize = static_cast<uint64_t>(size_);
    return write(socket_, &kConvertedSize, sizeof(uint64_t)) == sizeof(uint64_t);
}

bool dropbox::SocketExchange::ReceiveSize() {
    uint64_t received_size;

    const bool kCouldReceive = read(socket_, &received_size, sizeof(uint64_t)) == sizeof(uint64_t);
    if (!kCouldReceive) {
        return false;
    }

    size_ = static_cast<size_t>(received_size);
    return true;
}
