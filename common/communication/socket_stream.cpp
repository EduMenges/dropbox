#include "socket_stream.hpp"

#include <sys/socket.h>

#include <iostream>

std::streamsize dropbox::SocketBuffer::ReceiveData() {
    const ssize_t kBytesRead = ::recv(socket_, gptr(), kBufferSize, 0);

    if (kBytesRead == kInvalidRead) {
        perror(__func__);
    }

    return kBytesRead;
}

std::streamsize dropbox::SocketBuffer::SendData() {
    const size_t  kDataSize  = pptr() - pbase();
    const ssize_t kBytesSent = ::send(socket_, pbase(), kDataSize, 0);

    if (kBytesSent == -1) {
        throw std::system_error(std::error_code(errno, std::generic_category()));
    }

    pbump(-static_cast<int>(kDataSize));
    return kBytesSent;
}

int dropbox::SocketBuffer::underflow() {
    if (gptr() < egptr()) {
        return traits_type::to_int_type(*gptr());
    }

    auto bytes_read = ReceiveData();

    if (bytes_read == -1) {
        throw std::system_error(std::error_code(errno, std::generic_category()));
    }

    if (bytes_read == 0) {
        return traits_type::eof();
    }

    setg(buffer_->data(), buffer_->data(), buffer_->data() + bytes_read);
    return traits_type::to_int_type(*gptr());
}

int dropbox::SocketBuffer::overflow(int ch) {
    if (ch != traits_type::eof()) {
        *pptr() = traits_type::to_char_type(ch);
        pbump(1);
    }

    if (sync() == -1) {
        return traits_type::eof();
    }

    return ch;
}

void dropbox::SocketBuffer::InitializeBuffers() {
    buffer_ = std::make_unique<std::array<buffer_type, kBufferSize>>();
    setp(buffer_->data(), buffer_->data() + kBufferSize);
    setg(buffer_->data(), buffer_->data(), buffer_->data());
}
