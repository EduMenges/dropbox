#include "ring.hpp"

dropbox::Ring::Ring(const dropbox::Addr& my_addr) : prev_socket_(kInvalidSocket) {
    sockaddr_in receive_addr = my_addr.AsAddr();
    receive_addr.sin_addr    = {INADDR_ANY};

    if (bind(accept_socket_, reinterpret_cast<const sockaddr*>(&receive_addr), sizeof(sockaddr_in)) == -1) {
        throw Binding();
    }

    if (listen(accept_socket_, 1) == -1) {
        throw Listening();
    }

    [[maybe_unused]] bool res = accept_socket_.SetTimeout(kTimeout);
}

bool dropbox::Ring::AcceptPrev() noexcept {
    static sockaddr  prev_addr;
    static socklen_t prev_len;

    Socket new_prev(accept(accept_socket_, &prev_addr, &prev_len));

    if (!new_prev.IsValid()) {
        return false;
    }

    prev_socket_ = std::move(new_prev);
    return true;
}
