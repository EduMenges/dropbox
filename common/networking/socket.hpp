#pragma once

#include <utility>

#include "connections.hpp"
#include "tl/expected.hpp"
#include "unistd.h"

namespace dropbox {
class Socket {
   public:
    enum class KeepAliveError: uint8_t { kIdleTime, kMaxProbes, kTimeBetween, kEnable };

    class KeepAliveException : public std::system_error {
       public:
        using Error = std::pair<KeepAliveError, std::error_code>;

        explicit KeepAliveException(Error error) : error_(std::move(error)) {}

        [[nodiscard]] const char *what() const noexcept override;

        Error error_;
    };

    Socket() noexcept(false) : socket_(socket(kDomain, kType, kProtocol)) {
        if (socket_ == kInvalidSocket) {
            throw SocketCreation();
        }
    }

    explicit Socket(SocketType socket) : socket_(socket) {}

    Socket(Socket &&other) noexcept : socket_(std::exchange(other.socket_, kInvalidSocket)) {}

    Socket &operator=(const Socket &other) = delete;

    Socket &operator=(Socket &&other) noexcept {
        if (this != &other) {
            socket_ = std::exchange(other.socket_, kInvalidSocket);
        }

        return *this;
    }

    Socket(const Socket &other) = delete;

    ~Socket() {
        if (IsValid()) {
            close(socket_);
        }
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept { return socket_ != kInvalidSocket; }

    [[nodiscard]] bool HasConnection() const noexcept {
        static sockaddr_in address;
        socklen_t          address_len = sizeof(sockaddr_in);
        return getpeername(socket_, reinterpret_cast<sockaddr *>(&address), &address_len) == 0;
    }

    [[nodiscard]] bool Connect(const sockaddr_in &address) const noexcept {
        return connect(socket_, reinterpret_cast<const sockaddr *>(&address), sizeof(sockaddr_in)) == 0;
    }

    [[nodiscard]] tl::expected<void, std::pair<KeepAliveError, std::error_code>> SetKeepalive() const;

    constexpr operator int() const noexcept { return socket_; }

    SocketType socket_ = kInvalidSocket;
};

inline constexpr bool InvalidSockets(const Socket &socket) noexcept { return !socket.IsValid(); }

template <typename... Sockets>
inline constexpr bool InvalidSockets(const Socket &socket, const Sockets &...sockets) noexcept {
    return InvalidSockets(socket) || InvalidSockets(sockets...);
}

}
