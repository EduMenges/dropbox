#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <sys/types.h>

namespace dropbox {
    enum class Command {
        USERNAME,
        UPLOAD,
        DELETE,
        GET_SYNC_DIR,
        EXIT,
        LIST_SERVER
    };

    struct Header {
        Command command_;
        size_t payload_length_;
    };

    class Packet {
        public:
        
        inline Packet(int socket) : socket_(socket), payload_() {};

        inline constexpr void SetCommand(Command new_command) noexcept
        {
            header_.command_ = new_command;
        }

        ssize_t Send() const;
        ssize_t Receive();

        int socket_;
        Header header_;
        std::vector<uint8_t> payload_;
    };
}
