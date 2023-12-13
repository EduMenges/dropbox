#include "protocol.hpp"

#include <unistd.h>

#include <fstream>
#include <iostream>

#define CEREAL_THREAD_SAFE 1
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "utils.hpp"

thread_local std::array<char, dropbox::kPacketSize> dropbox::FileExchange::buffer;

bool dropbox::HeaderExchange::Send() {
    return write(socket_stream_.GetSocket(), &command_, sizeof(command_)) == SSizeOf<Command>();
}

bool dropbox::HeaderExchange::Receive() {
    return read(socket_stream_.GetSocket(), &command_, sizeof(command_)) == SSizeOf<Command>();
}

bool dropbox::FileExchange::Send() {
    std::basic_ifstream<char> file(path_, std::ios::in | std::ios::binary);

    if (!file.is_open() || file.bad()) {
        return false;
    }

    try {
        auto file_size = static_cast<int64_t>(std::filesystem::file_size(path_));

        cereal::PortableBinaryOutputArchive archive(socket_stream_);
        archive(file_size);

        do {
            const auto kBytesRead = file.read(buffer.data(), kPacketSize).gcount();
            socket_stream_.write(buffer.data(), kBytesRead);
        } while (!file.eof());

        return true;
    } catch (std::exception& e) {
        std::cerr << e.what();
        return false;
    }
}

bool dropbox::FileExchange::Receive() {
    if (!std::filesystem::is_regular_file(path_)) {
        std::filesystem::remove_all(path_);
    }

    std::basic_ofstream<char> file(path_, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        return false;
    }

    try {
        int64_t                            remaining_size;
        cereal::PortableBinaryInputArchive archive(socket_stream_);
        archive(remaining_size);

        while (remaining_size != 0) {
            const auto kBytesToReceive = std::min(remaining_size, static_cast<int64_t>(buffer.size()));
            const auto kBytesReceived  = socket_stream_.read(buffer.data(), kBytesToReceive).gcount();

            file.write(buffer.data(), kBytesReceived);
            remaining_size -= kBytesReceived;
        }

        return true;
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}

bool dropbox::EntryExchange::SendPath(const std::filesystem::path& path) {
    try {
        cereal::PortableBinaryOutputArchive archive(socket_stream_);
        archive(path.generic_string());
        return true;
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}

bool dropbox::EntryExchange::ReceivePath() {
    try {
        cereal::PortableBinaryInputArchive archive(socket_stream_);
        std::string                        recv_string;
        archive(recv_string);
        path_ = std::move(recv_string);
        return true;
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}
