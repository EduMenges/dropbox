#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include "constants.hpp"
#include "exceptions.hpp"

namespace dropbox {
using SocketType = int;

constexpr int kDomain   = AF_INET;
constexpr int kFamily   = kDomain;
constexpr int kType     = SOCK_STREAM;
constexpr int kProtocol = 0;

inline void MultipleConnect(const sockaddr_in *const kServerAddress, int socket) noexcept(false) {
    if (connect(socket, reinterpret_cast<const sockaddr *>(kServerAddress), sizeof(sockaddr_in)) == kInvalidConnect) {
        throw Connecting();
    }
}

template <typename... Sockets>
void MultipleConnect(const sockaddr_in *const kServerAddress, int socket, Sockets... sockets) noexcept(false) {
    MultipleConnect(kServerAddress, socket);
    MultipleConnect(kServerAddress, sockets...);
}

inline constexpr bool InvalidSockets(int socket) noexcept { return socket == kInvalidSocket; }

template <typename... Sockets>
inline constexpr bool InvalidSockets(int socket, Sockets... sockets) noexcept {
    return InvalidSockets(socket) || InvalidSockets(sockets...);
}

bool SetNonblocking(int fd);

bool SetTimeout(int socket, struct timeval timeout);
}
