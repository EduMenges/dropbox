#pragma once

#include "networking/Addr.hpp"
#include "networking/Socket.hpp"

namespace dropbox {
class Ring {
   public:
    Ring(const Addr &my_addr);

    Ring(Ring &&other) = default;

    Ring(const Ring &other) = delete;

    ~Ring() = default;

    [[nodiscard]] bool ConnectNext(const Addr &next_addr) const noexcept { return next_.Connect(next_addr.AsAddr()); }

    bool AcceptPrev() noexcept;

    [[nodiscard]] bool constexpr HasPrev() const noexcept { return prev_.IsValid(); }

    /// Tests whether there is a connection to the next in the ring
    [[nodiscard]] bool HasNext() const noexcept { return next_.HasConnection(); }

    static constexpr timeval kTimeout{2, 0};

    Socket accept_socket_;
    Socket next_;
    Socket prev_;
};
}
