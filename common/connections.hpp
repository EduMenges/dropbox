#pragma once

#include <sys/socket.h>

#include "exceptions.hpp"

namespace dropbox {
constexpr int kDomain   = AF_INET;
constexpr int kFamily   = kDomain;
constexpr int kType     = SOCK_STREAM;
constexpr int kProtocol = 0;

inline void MultipleConnect(const sockaddr_in * kServerAddress, int socket) noexcept(false)
{
    if (connect(socket,  reinterpret_cast<const sockaddr *>(kServerAddress), sizeof(sockaddr_in)) == kInvalidConnect)
    {
        throw Connecting();
    }
}

template <typename ...Sockets>
void MultipleConnect(const sockaddr_in * kServerAddress, int socket, Sockets ...sockets) noexcept(false)
{
    MultipleConnect(kServerAddress, socket);
    MultipleConnect(kServerAddress, sockets...);
}
}
