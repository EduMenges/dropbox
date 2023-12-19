#pragma once

#include <array>
#include <iostream>
#include <istream>
#include <memory>
#include <streambuf>
#include <utility>

#include "connections.hpp"
#include "constants.hpp"

namespace dropbox {

using BufferElementType = char;

class SocketBuffer : public std::basic_streambuf<BufferElementType> {
   public:
    static constexpr std::size_t kBufferSize = kPacketSize;

    inline SocketBuffer(SocketType socket)
        : socket_(socket), buffer_(std::make_unique<std::array<BufferElementType, kBufferSize>>()) {
        InitializePointers();
    }

    ~SocketBuffer() override = default;

    SocketBuffer(SocketBuffer&& other) noexcept
        : std::basic_streambuf<BufferElementType>(other),
          socket_(std::exchange(other.socket_, kInvalidSocket)),
          buffer_(std::move(other.buffer_)){};

    SocketBuffer(const SocketBuffer& other) = delete;

    [[nodiscard]] SocketType constexpr GetSocket() const noexcept {return socket_;}

   protected:
    int_type underflow() noexcept(false) override;

    int_type overflow(int_type ch) noexcept override;

    int sync() override;

    inline std::streamsize showmanyc() override { return egptr() - gptr(); }

   private:
    void InitializePointers();

    /// Abstracts the OS calls to send in socket_
    std::streamsize ReceiveData() noexcept;

    /// Abstracts the OS calls to receive in socket_
    std::streamsize SendData() noexcept;

    /// Socket descriptor that is not owned by the stream, therefore, is not destroyed with it.
    SocketType socket_;

    /// Underlying buffer for reading and writing operations.
    std::unique_ptr<std::array<BufferElementType, kBufferSize>> buffer_;
};

class SocketStream : public std::basic_iostream<BufferElementType> {
   public:
    explicit SocketStream(int socket) : std::basic_iostream<BufferElementType>(&buffer_), buffer_(socket){};
    SocketStream(const SocketStream& other) = delete;

    SocketStream(SocketStream&& other) noexcept;

    [[nodiscard]] SocketType GetSocket() const noexcept {return buffer_.GetSocket();}
   private:
    SocketBuffer buffer_;
};

}
