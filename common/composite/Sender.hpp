#pragma once

#include "networking/SocketStream.hpp"
#include "communication/protocol.hpp"
#include "composite/Base.hpp"
#include "networking/socket.hpp"

namespace dropbox::composite {
class Sender : public Base {
   public:
    explicit Sender(Socket&& socket) : Base(std::move(socket)) {}

    Sender(Sender&& other) = default;

    Sender(const Sender& other) = delete;

    ~Sender() override = default;

    virtual bool Upload(const std::filesystem::path& path);

    virtual bool Delete(const std::filesystem::path& path);
};
}
