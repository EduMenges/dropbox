#pragma once

#include <sys/types.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <utility>

#include "communication/commands.hpp"
#include "constants.hpp"
#include "socket_stream.hpp"

namespace dropbox {

/// Interface for exchanging information on both sides.
class Exchange {
   public:
    using SizeTy = size_t;

    Exchange()          = default;
    virtual ~Exchange() = default;

    /// Sending.
    [[nodiscard]] virtual bool Send() = 0;

    /// Receiving.
    [[nodiscard]] virtual bool Receive() = 0;
};

/// Exchanges between sockets.
class SocketExchange : public Exchange {
   public:
    inline SocketExchange() = default;

    inline SocketExchange(int socket) noexcept : socket_stream_(socket){};

    /**
     * Sets the internal socket.
     * @param socket Value to be the new socket.
     */
    inline void SetSocket(int socket) noexcept { socket_stream_.SetSocket(socket); }

    inline void Flush() {
        socket_stream_.flush();
    }

   protected:
    SocketStream socket_stream_;
    SizeTy size_{0};
};

/// Exchanges the header.
class HeaderExchange : public SocketExchange {
   public:
    inline HeaderExchange() = default;

    inline HeaderExchange(int socket) noexcept : SocketExchange(socket), command_{} {};

    inline HeaderExchange(HeaderExchange&& other) noexcept = default;

    /**
     * Sets the internal command.
     * @param new_command New command to set the internal with.
     * @return Instance of \c this to ease out method chaining.
     */
    inline constexpr HeaderExchange& SetCommand(Command new_command) noexcept {
        command_ = new_command;
        return *this;
    }

    /// Getter for the internal command.
    [[nodiscard]] inline constexpr Command GetCommand() const noexcept { return this->command_; }

    /// Sends \c this.
    [[nodiscard]] bool Send() override;

    /// Receives in \c this.
    [[nodiscard]] bool Receive() override;

   private:
    Command command_;  ///< Command to be exchanged.
};

/// Abstract class for exchanging entries (directories and files).
class EntryExchange : public SocketExchange {
   public:
    inline EntryExchange() = default;
    inline EntryExchange(int socket) : SocketExchange(socket) {}

    /**
     * Sends the internal path.
     * @return Operation status.
     */
    [[nodiscard]] inline bool SendPath() { return SendPath(path_); }

    /**
     * Receives a path.
     * @return Operation status.
     */
    [[nodiscard]] bool ReceivePath();

    /**
     * Sends a path.
     * @param path Path to be sent.
     * @return Operation status.
     */
    [[nodiscard]] bool SendPath(const std::filesystem::path& path);

    /**
     * Sets the internal path.
     * @param path Path to be setted.
     * @return Instance of \c this for method chaining.
     */
    inline EntryExchange& SetPath(std::filesystem::path&& path) {
        path_ = std::move(path);
        return *this;
    }

    /**
     * Getter for internal path.
     * @return Immutable internal path.
     */
    [[nodiscard]] inline const std::filesystem::path& GetPath() const { return path_; }

   protected:
    std::filesystem::path path_;  ///< The path to be exchanged.
};

/// Exchange whole files.
class FileExchange : public EntryExchange {
   public:
    FileExchange() = default;
    inline FileExchange(int socket) : EntryExchange(socket){};

    /// Sends the file under \ref path_
    [[nodiscard]] bool Send() override;

    /// Receives the file under \ref path_
    [[nodiscard]] bool Receive() override;

   private:
    /// Buffer to store the file in RAM with.
    static thread_local std::array<char, kPacketSize> buffer;  // NOLINT
};
}
