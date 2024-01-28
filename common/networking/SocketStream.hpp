#pragma once

#include <array>
#include <iostream>
#include <istream>
#include <memory>
#include <streambuf>
#include <utility>

#include "connections.hpp"
#include "constants.hpp"
#include "networking/Socket.hpp"

namespace dropbox {

using BufferElementType = char;

/// @c basic_streambuf usable with POSIX sockets
class SocketBuffer : public std::basic_streambuf<BufferElementType> {
   public:
    /// Size of the underlying buffer.
    static constexpr std::size_t kBufferSize = kPacketSize;

    /**
     * Constructor
     * @param socket Borrowed socket to send and receive data from.
     */
    explicit SocketBuffer(Socket& socket)
        : socket_(socket), buffer_(std::make_unique<std::array<BufferElementType, kBufferSize>>()) {
        InitializePointers();
    }

    SocketBuffer(SocketBuffer&& other) noexcept
        : std::basic_streambuf<BufferElementType>(other), socket_(other.socket_), buffer_(std::move(other.buffer_)){};

    SocketBuffer(const SocketBuffer& other) = delete;

    ~SocketBuffer() override = default;

    constexpr void SetSocket(Socket& socket) noexcept { socket_ = socket; }

   protected:
    int_type underflow() noexcept(false) override;

    int_type overflow(int_type ch) noexcept override;

    int sync() override;

    /* constexpr */ std::streamsize showmanyc() override { return egptr() - gptr(); }

   private:
    void InitializePointers();

    /// Abstracts the OS calls to send in @p socket_
    std::streamsize ReceiveData() noexcept;

    /// Abstracts the OS calls to receive in @p socket_
    std::streamsize SendData() noexcept;

    /// Socket descriptor that is not owned by the stream, therefore, is not destroyed with it.
    std::reference_wrapper<Socket> socket_;

    /// Underlying buffer for reading and writing operations.
    std::unique_ptr<std::array<BufferElementType, kBufferSize>> buffer_;
};

/// Stream to be used with TCP sockets.
class SocketStream : public std::basic_iostream<BufferElementType> {  // NOLINT
   public:
    explicit SocketStream(Socket& socket) : std::basic_iostream<BufferElementType>(&buffer_), buffer_(socket){};

    SocketStream(const SocketStream& other) = delete;

    SocketStream(SocketStream&& other) noexcept;

    void SetSocket(Socket& socket) { buffer_.SetSocket(socket); }

   private:
    SocketBuffer buffer_; ///< Underlying buffer.
};

}
