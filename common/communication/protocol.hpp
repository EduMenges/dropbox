#pragma once

#include <sys/types.h>

#include <array>
#include <cstdint>
#include <filesystem>

namespace dropbox {
enum class Command { USERNAME, UPLOAD, DELETE, GET_SYNC_DIR, EXIT, LIST_SERVER };

class Exchange {
   public:
    virtual ~Exchange()                  = default;
    [[nodiscard]] virtual bool Send()    = 0;
    [[nodiscard]] virtual bool Receive() = 0;
};

class HeaderExchange : public Exchange {
   public:
    inline constexpr HeaderExchange(int socket) noexcept : socket_(socket){};

    inline constexpr void SetCommand(Command new_command) noexcept { command_ = new_command; }

    [[nodiscard]] bool Send() override;
    [[nodiscard]] bool Receive() override;

   private:
    int     socket_;
    Command command_;
};

class EntryExchange : public Exchange {
   public:
    explicit EntryExchange(int socket) : socket_(socket) {}

    [[nodiscard]] bool SendPath();
    [[nodiscard]] bool ReceivePath();

    inline void SetPath(std::filesystem::path&& path) { path_ = std::move(path); }

    inline const std::filesystem::path GetPath() const { return path_; }

   protected:
    int                   socket_;
    std::filesystem::path path_;
};

class FileExchange : public EntryExchange {
   public:
    inline FileExchange(int socket) : EntryExchange(socket){};

    [[nodiscard]] bool Send() override;
    [[nodiscard]] bool Receive() override;

   private:
    static constexpr size_t kPacketSize = 64U * 1024U;

    int                                               socket_;
    std::unique_ptr<std::array<uint8_t, kPacketSize>> buffer_;
};

class DirectoryExchange : public EntryExchange {
   public:
    inline DirectoryExchange(int socket) : EntryExchange(socket){};

    bool Send() override;
    bool Receive() override;
    /// @todo This
   private:
    int socket_;
};
}
