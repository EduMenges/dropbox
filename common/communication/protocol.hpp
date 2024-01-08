#pragma once

#include <filesystem>

#include "commands.hpp"
#include "connections.hpp"
#include "networking/SocketStream.hpp"

namespace dropbox {
/// Uses raw sockets for communication
class SocketExchange {
   public:
    SocketExchange() : socket_(kInvalidSocket){};

    explicit SocketExchange(SocketType socket_fd) : socket_(socket_fd){};

    SocketExchange(SocketExchange&& other) noexcept : socket_(std::exchange(other.socket_, kInvalidSocket)){};

    [[nodiscard]] constexpr SocketType GetSocket() const noexcept { return socket_; }

   protected:
    SocketType socket_;
};

class FileExchange {
   public:
    FileExchange() = delete;

    explicit FileExchange(SocketStream& stream) : stream_(stream){};

    FileExchange(FileExchange&& other) = default;

    [[nodiscard]] const std::filesystem::path& GetPath() const noexcept { return path_; }

    FileExchange& SetPath(const std::filesystem::path& path) {
        path_ = path;
        return *this;
    }

    FileExchange& SetPath(std::filesystem::path&& path) {
        path_ = std::move(path);
        return *this;
    }

    void SendCommand(Command command) noexcept;

    std::optional<Command> ReceiveCommand() noexcept;

    bool SendPath() noexcept;

    bool ReceivePath() noexcept;

    bool Send() noexcept;

    bool Receive() noexcept;

    inline void Flush() { stream_.flush(); }

   private:
    /// Buffer to store the file in RAM with.
    static thread_local std::array<char, kPacketSize> buffer;  // NOLINT

    SocketStream& stream_;

    std::filesystem::path path_;
};
}
