#include "socket_stream.hpp"

#include <iostream>

dropbox::SocketStream::SocketStream(dropbox::SocketStream&& other) noexcept
    : basic_iostream<BufferElementType>(std::move(other)), buffer_(std::move(other.buffer_)) {
    this->set_rdbuf(&buffer_);
}

std::streamsize dropbox::SocketBuffer::ReceiveData() noexcept {
    const ssize_t kBytesRead = ::read(socket_, buffer_->begin(), kBufferSize);

    if (kBytesRead == kInvalidRead) {
        perror(__func__);
    }

    return static_cast<std::streamsize>(kBytesRead);
}

std::streamsize dropbox::SocketBuffer::SendData() noexcept {
    const size_t  kDataSize  = pptr() - pbase();
    const ssize_t kBytesSent = ::write(socket_, pbase(), kDataSize);

    if (kBytesSent == kInvalidWrite) {
        perror(__func__);
    }

    return static_cast<std::streamsize>(kBytesSent);
}

int dropbox::SocketBuffer::underflow() noexcept(false) {
    if (gptr() < egptr()) {
        return traits_type::to_int_type(*gptr());
    }

    sync();

    const auto kBytesReceived = ReceiveData();

    if (kBytesReceived == kInvalidRead) {
        throw std::system_error(std::error_code(errno, std::generic_category()));
    }

    if (kBytesReceived == 0) {
        return traits_type::eof();
    }

    setg(buffer_->begin(), buffer_->begin(), buffer_->begin() + kBytesReceived);
    return traits_type::to_int_type(*gptr());
}

int dropbox::SocketBuffer::overflow(int ch) noexcept {
    if (ch != traits_type::eof()) {
        *pptr() = traits_type::to_char_type(ch);
        pbump(1);
    }

    if (sync() == -1) {
        return traits_type::eof();
    }

    return ch;
}

void dropbox::SocketBuffer::InitializePointers() {
    setp(buffer_->begin(), buffer_->end());
    setg(buffer_->begin(), buffer_->begin(), buffer_->begin());
}

int dropbox::SocketBuffer::sync() {
    const auto kTotalSent = SendData();

    if (kTotalSent == kInvalidWrite) {
        return -1;
    }

    pbump(static_cast<int>(-kTotalSent));
    return 0; }
