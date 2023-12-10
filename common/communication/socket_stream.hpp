#pragma once

#include <array>
#include <istream>
#include <memory>
#include <streambuf>

#include "constants.hpp"

namespace dropbox {

using buffer_type = char;

class SocketBuffer : public std::basic_streambuf<buffer_type> {
   public:
    static constexpr std::size_t kBufferSize = kPacketSize;
    SocketBuffer(int socket) : socket_(socket) { InitializeBuffers(); }

   protected:
    int_type underflow() override;

    int_type overflow(int_type ch) override;

    inline int sync() override { return static_cast<int>(SendData()); }

   private:
    void InitializeBuffers();

    std::streamsize ReceiveData();

    std::streamsize SendData();

    int                                                   socket_;
    std::unique_ptr<std::array<buffer_type, kBufferSize>> buffer_;
};

class SocketStream : public std::basic_iostream<buffer_type> {
   public:
    explicit SocketStream(int socket) : std::basic_iostream<buffer_type>(&buffer_), buffer_(socket) {}

   private:
    SocketBuffer buffer_;
};

}
