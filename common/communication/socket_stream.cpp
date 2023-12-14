#include "socket_stream.hpp"

#include <filesystem>
#include <iostream>

#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"

std::streamsize dropbox::SocketBuffer::ReceiveData() noexcept {
    const ssize_t kBytesRead = ::read(socket_, gptr(), kBufferSize);

    if (kBytesRead == kInvalidRead) {
        perror(__func__);
    }

    return static_cast<std::streamsize>(kBytesRead);
}

std::streamsize dropbox::SocketBuffer::SendData() noexcept {
    const size_t  kDataSize  = pptr() - pbase();
    const ssize_t kBytesSent = ::write(socket_, pbase(), kDataSize);

    if (kBytesSent == kInvalidWrite) {
        return kInvalidWrite;
    }

    pbump(-static_cast<int>(kDataSize));
    return static_cast<std::streamsize>(kBytesSent);
}

int dropbox::SocketBuffer::underflow() noexcept(false) {
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
    setp(buffer_->data(), buffer_->data() + kBufferSize);
    setg(buffer_->data(), buffer_->data(), buffer_->data());
}
