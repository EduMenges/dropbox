#pragma once

#include "networking/Addr.hpp"
#include "networking/Socket.hpp"

namespace dropbox {
class Ring {
   public:
    explicit Ring(const Addr &my_addr);

    Ring(Ring &&other) = default;

    Ring(const Ring &other) = delete;

    ~Ring() = default;

    /**
     * Connects the next socket.
     * @param next_addr Where to connect to.
     * @return Whether it could connect.
     */
    [[nodiscard]] bool ConnectNext(const Addr &next_addr) const noexcept { return next_.Connect(next_addr.AsAddr()); }

    /**
     * Accepts a previous socket.
     * @warning @c false may be due to timeout, not due to error. Check errno!
     * @return Whether a previous connection happened.
     */
    bool AcceptPrev();

    [[nodiscard]] bool  HasPrev() const noexcept { return prev_.HasConnection(); }

    /// Tests whether there is a connection to the next in the ring.
    [[nodiscard]] bool HasNext() const noexcept { return next_.HasConnection(); }

    /// Timeout used in sockets to enable graceful exit.
    static constexpr timeval kTimeout{2, 0};

    Socket accept_socket_; ///< Socket to listen to new connections for @p prev_.
    Socket prev_; ///< Previous replica in the ring.
    Socket next_; ///< Next replica in the ring.
};
}
