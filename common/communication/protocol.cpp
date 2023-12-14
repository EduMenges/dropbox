#include "protocol.hpp"

#include <unistd.h>

#include <fstream>

#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"

thread_local std::array<char, dropbox::kPacketSize> dropbox::FileExchange::buffer;

bool dropbox::HeaderExchange::Send(dropbox::Command command) noexcept {
    const ssize_t kSentBytes = ::write(socket_, &command, sizeof(Command));

    if (kSentBytes != sizeof(Command)) {
        if (kSentBytes != 0) {
            perror("HeaderExchange::Send");
        }
        return false;
    }

    return true;
}

std::optional<dropbox::Command> dropbox::HeaderExchange::Receive() noexcept {
    Command       command;
    const ssize_t kReceivedBytes = ::read(socket_, &command, sizeof(Command));

    if (kReceivedBytes != sizeof(Command)) {
        if (kReceivedBytes != 0) {
            perror("HeaderExchange::Receive");
        }
        return std::nullopt;
    }

    return command;
}

bool dropbox::FileExchange::SendPath() noexcept {
    try {
        cereal::PortableBinaryOutputArchive archive(*stream_);
        archive(path_.generic_string());
        return true;
    } catch (std::exception& e) {
        std::cerr << "Error when sending path: " << e.what() << '\n';
        return false;
    }
}

bool dropbox::FileExchange::ReceivePath() noexcept {
    try {
        std::string                        in_str;
        cereal::PortableBinaryInputArchive archive(*stream_);
        archive(in_str);
        SetPath(std::move(in_str));
        return true;
    } catch (std::exception& e) {
        std::cerr << "Error when receiving path: " << e.what() << '\n';
        return false;
    }
}

bool dropbox::FileExchange::Send() noexcept {
    std::basic_ifstream<char> file(path_, std::ios::in | std::ios::binary);

    if (!file.is_open() || file.bad()) {
        return false;
    }

    try {
        auto file_size = static_cast<int64_t>(std::filesystem::file_size(path_));

        cereal::PortableBinaryOutputArchive archive(*stream_);
        archive(file_size);

        do {
            const auto kBytesRead = file.read(buffer.data(), kPacketSize).gcount();
            stream_->write(buffer.data(), kBytesRead);
        } while (!file.eof());

        return true;
    } catch (std::exception& e) {
        std::cerr << "FileExchange::Send: " << e.what();
        return false;
    }
}

bool dropbox::FileExchange::Receive() noexcept {
    if (!std::filesystem::is_regular_file(path_)) {
        std::filesystem::remove_all(path_);
    }

    std::basic_ofstream<char> file(path_, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    try {
        int64_t remaining_size = 0;

        cereal::PortableBinaryInputArchive archive(*stream_);
        archive(remaining_size);

        while (remaining_size != 0) {
            const auto kBytesToReceive = std::min(remaining_size, static_cast<intmax_t>(buffer.size()));
            const auto kBytesReceived  = stream_->read(buffer.data(), kBytesToReceive).gcount();

            if (kBytesReceived == kInvalidRead) {
                return false;
            }

            file.write(buffer.data(), kBytesReceived);
            remaining_size -= kBytesReceived;
        }

        return true;
    } catch (std::exception& e) {
        std::cerr << "FileExchange::Receive: " << e.what() << '\n';
        return false;
    }
}
