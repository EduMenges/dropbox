#include "socket.hpp"
#include <netinet/tcp.h>

bool dropbox::Socket::SetKeepalive() const {
    constexpr auto kFlagsLen = static_cast<socklen_t>(sizeof(int));

    constexpr int kIdleTime = 5;
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPIDLE, &kIdleTime, kFlagsLen) == -1) {
        return false;
    }

    constexpr int kMaxProbesBeforeDisconnection = 5;
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPCNT, &kMaxProbesBeforeDisconnection, kFlagsLen) == -1) {
        return false;
    }

    constexpr int kTimeBetweenProbes = 1;
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPINTVL, &kTimeBetweenProbes, kFlagsLen) == -1) {
        return false;
    }

    constexpr int kOn = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &kOn, kFlagsLen) == -1) {
        return false; //NOLINT
    }

    return true;
}
