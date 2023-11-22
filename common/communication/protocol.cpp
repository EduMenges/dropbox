#include "protocol.hpp"

#include <unistd.h>

#include <climits>
#include <fstream>

bool dropbox::HeaderExchange::Send() {
    return write(socket_, &this->command_, sizeof(this->command_)) == sizeof(this->command_);
}

bool dropbox::HeaderExchange::Receive() {
    return read(socket_, &this->command_, sizeof(this->command_)) == sizeof(this->command_);
}

bool dropbox::FileExchange::Send() {
    if (!SendPath()) {
        return false;
    }

    std::basic_ifstream<uint8_t> file(path_, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    while (!file.eof()) {
        const auto    kBytesRead = static_cast<size_t>(file.read(buffer_->data(), kPacketSize).gcount());
        const ssize_t kBytesSent = write(socket_, buffer_->data(), kBytesRead);

        if (kBytesSent < kBytesRead) {
            return false;
        }
    }

    return true;
}

bool dropbox::FileExchange::Receive() {
    if (!ReceivePath()) {
        return false;
    }

    if (!std::filesystem::is_regular_file(path_)) {
        std::filesystem::remove_all(path_);
    }

    std::basic_ofstream<uint8_t> file(path_, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    do {
        const auto kBytesReceived = read(socket_, buffer_->data(), kPacketSize);

        if (kBytesReceived == -1)
        {
            return false;
        }

        file.write(buffer_->data(), kBytesReceived);
    } while (!file.eof());

    return true;
}

bool dropbox::EntryExchange::SendPath() {
    const auto kConvertedPath = path_.generic_u8string();
    return write(socket_, kConvertedPath.data(), kConvertedPath.size()) == kConvertedPath.size();
}

bool dropbox::EntryExchange::ReceivePath() {
    static std::u8string received_path(PATH_MAX, u8'\0');

    if (read(socket_, received_path.data(), PATH_MAX) == -1)
    {
        return false;
    }

    path_ = received_path;
    return true;
}

bool dropbox::DirectoryExchange::Send() { return false; }
bool dropbox::DirectoryExchange::Receive() { return false; }
