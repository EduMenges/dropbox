#include "Socket.hpp"
#include "fmt/core.h"
#include "fmt/format.h"
#include <netinet/tcp.h>
#include <poll.h>

tl::expected<void, std::pair<dropbox::Socket::KeepAliveError, std::error_code>> dropbox::Socket::SetKeepalive()
    const noexcept {
    constexpr auto kFlagsLen = static_cast<socklen_t>(sizeof(int));

    constexpr int kIdleTime = 5;
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPIDLE, &kIdleTime, kFlagsLen) == -1) {
        return tl::make_unexpected(
            std::make_pair(KeepAliveError::kIdleTime, std::make_error_code(static_cast<std::errc>(errno))));
    }

    constexpr int kMaxProbesBeforeDisconnection = 5;
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPCNT, &kMaxProbesBeforeDisconnection, kFlagsLen) == -1) {
        return tl::make_unexpected(
            std::make_pair(KeepAliveError::kMaxProbes, std::make_error_code(static_cast<std::errc>(errno))));
    }

    constexpr int kTimeBetweenProbes = 1;
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPINTVL, &kTimeBetweenProbes, kFlagsLen) == -1) {
        return tl::make_unexpected(
            std::make_pair(KeepAliveError::kTimeBetween, std::make_error_code(static_cast<std::errc>(errno))));
    }

    constexpr int kOn = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &kOn, kFlagsLen) == -1) {
        return tl::make_unexpected(
            std::make_pair(KeepAliveError::kEnable, std::make_error_code(static_cast<std::errc>(errno))));
    }

    return {};
}

bool dropbox::Socket::HasConnection() const noexcept {
    auto result = recv(socket_, nullptr, 1, MSG_DONTWAIT | MSG_PEEK);

    if (result == -1) {
        return errno == EWOULDBLOCK || errno == EFAULT;
    }

    return result != 0;
}

template <>
struct fmt::formatter<dropbox::Socket::KeepAliveError> : formatter<string_view> {
   public:
    constexpr auto format(dropbox::Socket::KeepAliveError err, format_context& ctx) const {
        std::string_view name;

        switch (err) {
            case dropbox::Socket::KeepAliveError::kIdleTime:
                name = "kIdleTime";
                break;
            case dropbox::Socket::KeepAliveError::kMaxProbes:
                name = "kMaxProbes";
                break;
            case dropbox::Socket::KeepAliveError::kTimeBetween:
                name = "kTimeBetween";
                break;
            case dropbox::Socket::KeepAliveError::kEnable:
                name = "kEnable";
        }

        return formatter<string_view>::format(name, ctx);
    }
};

const char* dropbox::Socket::KeepAliveException::what() const noexcept {
    static thread_local std::string result;
    result = fmt::format("SetKeepAlive::{} {}", error_.first, error_.second.message());
    return result.c_str();
}
