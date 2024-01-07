#pragma once

#include "networking/socket.hpp"
#include "communication/protocol.hpp"
#include "networking/SocketStream.hpp"

namespace dropbox::composite {
class Base {
   public:
    explicit Base(Socket&& socket) : socket_(std::move(socket)), stream_(socket_), exchange_(stream_) {}

    Base(Base&& other) = default;

    Base(const Base& other) = delete;

    virtual ~Base() = default;

    Socket       socket_;
    SocketStream stream_;
    FileExchange exchange_;
};
}
