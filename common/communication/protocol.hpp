#pragma once

#include <sys/types.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>

namespace dropbox {

/// Possible user actions
enum class Command {
    UPLOAD,        ///< Uploads a file at the root directory.
    DELETE,        ///< Deletes a file at the root directory.
    USERNAME, ///< Username receiver.
    GET_SYNC_DIR,  ///< Downloads the \c sync_dir directory and starts syncing.
    EXIT,          ///< Ends connection with server
    LIST_CLIENT,   ///< Lists the files from the client
    LIST_SERVER,   ///< Lists the files from the server
    DOWNLOAD,      ///< Downloads a file to the \c cwd.
    ERROR, ///< An error occurred.
    SUCCESS ///< Operation was a success.
};

/// Interface for exchanging information on both sides.
class Exchange {
   public:
    virtual ~Exchange() = default;

    /// Sending.
    [[nodiscard]] virtual bool Send() = 0;

    /// Receiving.
    [[nodiscard]] virtual bool Receive() = 0;
};

std::ostream& operator<<(std::ostream& os, Command command);

/// Exchanges the header.
class HeaderExchange : public Exchange {
   public:
    inline constexpr HeaderExchange() : socket_(-1), command_{} {};

    inline constexpr HeaderExchange(int socket) noexcept : socket_(socket), command_{} {};

    inline constexpr HeaderExchange(int socket, Command command) noexcept : socket_(socket), command_(command){};

    inline constexpr HeaderExchange& SetCommand(Command new_command) noexcept {
        command_ = new_command;
        return *this;
    }

    inline void constexpr SetSocket(int socket) noexcept { socket_ = socket; }

    [[nodiscard]] bool Send() override;
    [[nodiscard]] bool Receive() override;

    [[nodiscard]] inline constexpr Command GetCommand() const noexcept { return this->command_; }

   private:
    int     socket_;   ///< Where send and receive from.
    Command command_;  ///< Command to be exchanged.
};

/// Abstract class for exchanging entries (directories and files).
class EntryExchange : public Exchange {
   public:
    inline EntryExchange() : socket_(-1) {}
    inline EntryExchange(int socket) : socket_(socket) {}

    [[nodiscard]] inline bool SendPath() const { return SendPath(path_); }

    [[nodiscard]] bool ReceivePath();

    [[nodiscard]] bool SendPath(const std::filesystem::path& path) const;

    inline void constexpr SetSocket(int socket) noexcept { socket_ = socket; }

    inline EntryExchange& SetPath(std::filesystem::path&& path) {
        path_ = std::move(path);
        return *this;
    }

    [[nodiscard]] inline const std::filesystem::path& GetPath() const { return path_; }

   protected:
    int                   socket_;  ///< Where send and receive from.
    std::filesystem::path path_;    ///< The path to be exchanged.
};

/// Exchange whole files.
class FileExchange : public EntryExchange {
   public:
    FileExchange() = default;
    inline FileExchange(int socket) : EntryExchange(socket){};

    [[nodiscard]] bool Send() override;
    [[nodiscard]] bool Receive() override;

   private:
    /// Max size of a single packet exchange.
    static constexpr size_t kPacketSize = 64U * 1024U;

    /// Buffer to store the file in RAM with.
    static thread_local std::array<char, kPacketSize> buffer;
};

/// Exchanges whole directories.
class DirectoryExchange : public EntryExchange {
   public:
    DirectoryExchange() = default;
    inline DirectoryExchange(int socket) : EntryExchange(socket){};

    bool Send() override;
    bool Receive() override;
    /// @todo This
};
}
