#pragma once

#include <sys/types.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>

namespace dropbox {
enum class Command { USERNAME, UPLOAD, DELETE, GET_SYNC_DIR, EXIT, LIST_SERVER, DOWNLOAD, LIST_CLIENT };

class Exchange {
   public:
    virtual ~Exchange()                  = default;
    [[nodiscard]] virtual bool Send()    = 0;
    [[nodiscard]] virtual bool Receive() = 0;
};

std::ostream& operator<<(std::ostream& os, Command command);

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

    Command GetCommand();

   private:
    int     socket_;
    Command command_;
};

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

    inline const std::filesystem::path& GetPath() const { return path_; }

   protected:
    int                   socket_;
    std::filesystem::path path_;
};

class FileExchange : public EntryExchange {
   public:
    FileExchange() = default;
    inline FileExchange(int socket) : EntryExchange(socket){};

    [[nodiscard]] bool Send() override;
    [[nodiscard]] bool Receive() override;

   private:
    /// Max size of a single packet exchange.
    static constexpr size_t kPacketSize = 64U * 1024U;

    static thread_local std::array<char, kPacketSize> buffer;
};

class DirectoryExchange : public EntryExchange {
   public:
    DirectoryExchange() = default;
    inline DirectoryExchange(int socket) : EntryExchange(socket){};

    bool Send() override;
    bool Receive() override;
    /// @todo This
};
}
