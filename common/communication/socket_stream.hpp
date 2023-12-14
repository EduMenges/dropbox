#pragma once

#include <array>
#include <istream>
#include <memory>
#include <streambuf>

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

    inline SocketBuffer()
        : socket_(kInvalidSocket), buffer_(std::make_unique<std::array<BufferElementType, kBufferSize>>()) {
        InitializePointers();
    };

    ~SocketBuffer() override                = default;
    SocketBuffer(SocketBuffer&& other)      = default;
    SocketBuffer(const SocketBuffer& other) = delete;

    inline constexpr void SetSocket(SocketType socket) noexcept { socket_ = socket; }

    [[nodiscard]] inline constexpr int GetSocket() const noexcept { return socket_; }

   protected:
    int_type underflow() noexcept(false) override;

    int_type overflow(int_type ch) noexcept override;

    inline int sync() override {
        const auto kTotalSent = SendData();

        if (kTotalSent == kInvalidWrite) {
            return -1;
        }

        return 0;
    }

    inline std::streamsize showmanyc() override { return egptr() - gptr(); }

   private:
    void InitializePointers();

    /// Abstracts the OS calls to send in @ref socket_
    std::streamsize ReceiveData() noexcept;

    /// Abstracts the OS calls to receive in @ref socket_
    std::streamsize SendData() noexcept;

    /// Socket descriptor that is not owned by the stream, therefore, is not destroyed with it.
    SocketType socket_;

    /// Underlying buffer for reading and writing operations.
    std::unique_ptr<std::array<BufferElementType, kBufferSize>> buffer_;
};

class SocketStream : public std::basic_iostream<BufferElementType> {
   public:
    explicit SocketStream(int socket) : std::basic_iostream<BufferElementType>(&buffer_), buffer_(socket){};
    SocketStream() : std::basic_iostream<BufferElementType>(&buffer_){};

    inline SocketStream(SocketStream&& other) noexcept
        : basic_iostream<BufferElementType>(std::move(other)), buffer_(std::move(other.buffer_)){};

    inline void SetSocket(int socket) noexcept { buffer_.SetSocket(socket); }

   private:
    SocketBuffer buffer_;
};

}
