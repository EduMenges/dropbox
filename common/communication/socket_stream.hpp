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
    SocketBuffer() : socket_(kInvalidSocket) { InitializeBuffers(); };

    ~SocketBuffer() override                = default;
    SocketBuffer(SocketBuffer&& other)      = default;
    SocketBuffer(const SocketBuffer& other) = delete;

    inline constexpr void SetSocket(int socket) noexcept { socket_ = socket; }

    inline constexpr int GetSocket() const noexcept{
        return socket_;
    }

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

   private:
    void InitializeBuffers();

    std::streamsize ReceiveData() noexcept;

    std::streamsize SendData() noexcept;

    int                                                   socket_;
    std::unique_ptr<std::array<buffer_type, kBufferSize>> buffer_;
};

class SocketStream : public std::basic_iostream<buffer_type> {
   public:
    explicit SocketStream(int socket) : std::basic_iostream<buffer_type>(&buffer_), buffer_(socket){};
    SocketStream() : std::basic_iostream<buffer_type>(&buffer_){};

    inline SocketStream(SocketStream&& other) noexcept
        : basic_iostream<buffer_type>(std::move(other)), buffer_(std::move(other.buffer_)){};

    inline void SetSocket(int socket) noexcept { buffer_.SetSocket(socket); }

    inline int GetSocket() noexcept {
        return buffer_.GetSocket();
    }

   private:
    SocketBuffer buffer_;
};

}
