#pragma once

#include "networking/addr.hpp"
#include "networking/socket.hpp"

namespace dropbox {
class Ring {
   public:
    Ring(const Addr &my_addr);

    Ring(Ring &&other) = default;

    Ring(const Ring &other) = delete;

    [[nodiscard]] bool ConnectNext(const Addr &next_addr) const noexcept {
        return connect(next_socket_, reinterpret_cast<const sockaddr *>(&next_addr.AsAddr()), sizeof(sockaddr_in)) !=
               -1;
    }

    bool AcceptPrev() noexcept;

    [[nodiscard]] bool constexpr HasPrev() const noexcept { return prev_socket_ != kInvalidSocket; }

    /// Tests whether there is a connection to the next in the ring
    [[nodiscard]] bool HasNext() const noexcept { return next_socket_.HasConnection(); }

    ~Ring() = default;

    Socket next_socket_;
    Socket prev_socket_;

   private:
    static constexpr timeval kTimeout{1, 0};
    Socket                   accept_socket_;
};
}
