#pragma once

#include "networking/Socket.hpp"
#include "communication/protocol.hpp"
#include "networking/SocketStream.hpp"

namespace dropbox::composite {
class Base {
   public:
    explicit Base(Socket&& socket) : socket_(std::move(socket)), stream_(socket_), exchange_(stream_) {}

    Base(Base&& other) noexcept
        : socket_(std::move(other.socket_)), stream_(std::move(other.stream_)), exchange_(std::move(other.exchange_)) {
        stream_.SetSocket(socket_);
        exchange_.SetStream(stream_);
    }

    Base(const Base& other) = delete;

    virtual ~Base() = default;

    Socket       socket_;
    SocketStream stream_;
    FileExchange exchange_;
};
}
