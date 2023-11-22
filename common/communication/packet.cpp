#include "packet.hpp"

#include <unistd.h>

#include "utils.hpp"

ssize_t dropbox::Packet::Send() const { 
    const ssize_t kHeaderResult = write(socket_, &this->header_, sizeof(this->header_));

    if (kHeaderResult == -1)
    {
        return kHeaderResult;
    }

    if (this->header_.command_ == Command::UPLOAD)
    {
        const ssize_t kFileResult = write(socket_, this->payload_.data(), TotalSize(this->payload_));
        if (kFileResult == -1)
        {
            return kFileResult;
        }
        else
        {
            return kHeaderResult + kFileResult;
        }
    }
    else
    {
        return kHeaderResult;
    }
}

ssize_t dropbox::Packet::Receive() {
    const ssize_t kHeaderResult = read(socket_, &this->header_, sizeof(this->header_));

    if (kHeaderResult == -1)
    {
        return kHeaderResult;
    }

    if (this->header_.command_ == Command::UPLOAD)
    {
        this->payload_.reserve(header_.payload_length_);

        const ssize_t kFileResult = read(socket_, this->payload_.data(), header_.payload_length_);
        if (kFileResult == -1)
        {
            return kFileResult;
        }
        else
        {
            return kHeaderResult + kFileResult;
        }
    }
    else
    {
        return kHeaderResult;
    }
}