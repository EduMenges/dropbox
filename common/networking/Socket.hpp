#pragma once

#include <utility>

#include "connections.hpp"
#include "tl/expected.hpp"
#include "unistd.h"
#include "fmt/core.h"

namespace dropbox {
/// Socket class that uses RAII.
class Socket {
   public:
    /// Errors when setting keepalive.
    enum class KeepAliveError : uint8_t { kIdleTime, kMaxProbes, kTimeBetween, kEnable };

    /// Exception when setting keepalive.
    class KeepAliveException : public std::system_error {
       public:
        using Error = std::pair<KeepAliveError, std::error_code>;

        explicit KeepAliveException(Error error) : error_(std::move(error)) {}

        [[nodiscard]] const char *what() const noexcept override;

        Error error_;
    };

    /// Exception when setting timeout.
    class SetTimeoutException : public std::system_error {
       public:
        SetTimeoutException() = default;

        [[nodiscard]] const char *what() const noexcept override { return "Error in setting timeout"; }
    };

    /// Creates a TCP socket with a valid descriptor.
    Socket() noexcept(false) : socket_(socket(kDomain, kType, kProtocol)) {
        if (socket_ == kInvalidSocket) {
            throw SocketCreation();
        }
    }

    /// @pre Assumes that @p socket is now OWNED by this instance, therefore, is closed with it.
    explicit Socket(SocketType socket) : socket_(socket) {}

    Socket(const Socket &other) = delete;

    Socket(Socket &&other) noexcept : socket_(std::exchange(other.socket_, kInvalidSocket)) {}

    Socket &operator=(const Socket &other) = delete;

    Socket &operator=(Socket &&other) noexcept {
        if (this != &other) {
            socket_ = std::exchange(other.socket_, kInvalidSocket);
        }

        return *this;
    }

    ~Socket() {
        if (IsValid()) {
            close(socket_);
        }
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept { return socket_ != kInvalidSocket; }

    /// @return Whether the socket has a connection.
    [[nodiscard]] bool HasConnection() const noexcept {
        static sockaddr_in address;
        socklen_t          address_len = sizeof(sockaddr_in);
        return getpeername(socket_, reinterpret_cast<sockaddr *>(&address), &address_len) == 0;
    }

    [[nodiscard]] bool Bind(const sockaddr_in &addr) const noexcept {
        return bind(socket_, reinterpret_cast<const sockaddr *>(&addr), sizeof(sockaddr_in)) == 0;
    }

    [[nodiscard]] bool Listen(int backlog) const noexcept { return listen(socket_, backlog) == 0; }

    [[nodiscard]] bool Connect(const sockaddr_in &address) const noexcept {
        return connect(socket_, reinterpret_cast<const sockaddr *>(&address), sizeof(sockaddr_in)) == 0;
    }

    [[nodiscard]] tl::expected<void, std::pair<KeepAliveError, std::error_code>> SetKeepalive() const noexcept;

    bool SetTimeout(struct timeval timeout) const { return SetOpt(SOL_SOCKET, SO_RCVTIMEO, timeout); }

    template <typename O>
    bool SetOpt(int level, int optname, O optval) const {
        return setsockopt(socket_, level, optname, &optval, sizeof(O)) == 0;
    }

    /**
     * @post Socket is not closed since it is only borrowed.
     * @return The underlying socket descriptor.
     */
    [[nodiscard]] constexpr SocketType Get() const noexcept { return socket_; }

   private:
    SocketType socket_ = kInvalidSocket;  ///< Underlying, implementation-defined socket.
};

inline constexpr bool InvalidSockets(const Socket &socket) noexcept { return !socket.IsValid(); }

template <typename... Sockets>
inline constexpr bool InvalidSockets(const Socket &socket, const Sockets &...sockets) noexcept {
    return InvalidSockets(socket) || InvalidSockets(sockets...);
}

}
