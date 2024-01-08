#include "protocol.hpp"

#include <fstream>

#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"

thread_local std::array<char, dropbox::kPacketSize> dropbox::FileExchange::buffer;

bool dropbox::FileExchange::SendPath() noexcept {
    try {
        cereal::PortableBinaryOutputArchive archive(stream_);
        const auto                          kSent = path_.generic_u8string();
        archive(kSent);

        return true;
    } catch (std::exception& e) {
        fmt::println(stderr, "{}: {}", __func__, e.what());
        return false;
    }
}

bool dropbox::FileExchange::ReceivePath() noexcept {
    try {
        cereal::PortableBinaryInputArchive archive(stream_);
        std::u8string                      in_str;
        archive(in_str);

        SetPath(std::filesystem::path(in_str));

        return true;
    } catch (std::exception& e) {
        fmt::println(stderr, "{}: {}", __func__, e.what());
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

        cereal::PortableBinaryOutputArchive archive(stream_);
        archive(file_size);

        do {
            const auto kBytesRead = file.read(buffer.data(), kPacketSize).gcount();
            stream_.write(buffer.data(), kBytesRead);
        } while (!file.eof());

        return true;
    } catch (std::exception& e) {
        fmt::println(stderr, "{}: {}", __func__, e.what());
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

        cereal::PortableBinaryInputArchive archive(stream_);
        archive(remaining_size);

        while (remaining_size != 0) {
            const auto kBytesToReceive = std::min(remaining_size, static_cast<intmax_t>(buffer.size()));
            const auto kBytesReceived  = stream_.read(buffer.data(), kBytesToReceive).gcount();

            if (kBytesReceived == kInvalidRead) {
                return false;
            }

            file.write(buffer.data(), kBytesReceived);
            remaining_size -= kBytesReceived;
        }

        return true;
    } catch (std::exception& e) {
        fmt::println(stderr, "{}: {}", __func__, e.what());
        return false;
    }
}

std::optional<dropbox::Command> dropbox::FileExchange::ReceiveCommand() noexcept {
    try {
        cereal::PortableBinaryInputArchive archive(stream_);
        Command                            command;
        archive(command);

        return command;
    } catch (std::exception& e) {
        fmt::println(stderr, "{}: {}", __func__, e.what());
        return std::nullopt;
    }
}

void dropbox::FileExchange::SendCommand(dropbox::Command command) noexcept {
    cereal::PortableBinaryOutputArchive archive(stream_);
    archive(command);
}
