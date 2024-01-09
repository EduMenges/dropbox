#pragma once

#include <filesystem>

#include "commands.hpp"
#include "connections.hpp"
#include "networking/SocketStream.hpp"

namespace dropbox {
/// Class to exchange files, paths, and commands
class FileExchange {
   public:
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

    tl::expected<dropbox::Command, std::error_code> ReceiveCommand() noexcept;

    bool SendPath() noexcept;

    bool ReceivePath() noexcept;

    /**
     * Sends the file stored at @p path_.
     * @return Whether it could send the file.
     */
    bool Send() noexcept;

    /**
     * Receives the file stored at @p path_;
     * @return Whether it could receive the file.
     */
    bool Receive() noexcept;

    /**
     * Sends all the content stored in the stream.
     */
    void Flush() { stream_.flush(); }

   private:
    /// Buffer to store the file in RAM with.
    static thread_local std::array<char, kPacketSize> buffer;  // NOLINT

    SocketStream& stream_; ///< Stream to send and receive from.

    std::filesystem::path path_; ///< File path.
};
}
