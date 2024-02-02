#include "Ring.hpp"
#include "fmt/core.h"

dropbox::Ring::Ring(const dropbox::Addr& my_addr) : prev_(kInvalidSocket), next_(kInvalidSocket) {
    sockaddr_in receive_addr = my_addr.AsAddr();
    receive_addr.sin_addr    = {INADDR_ANY};

    if (!accept_socket_.Bind(receive_addr)) {
        fmt::println(stderr, "{}: when binding to accept_socket", __func__);
        throw Binding();
    }

    if (!accept_socket_.Listen(1)) {
        throw Listening();
    }

    if (!accept_socket_.SetTimeout(kTimeout)) {
        throw Socket::SetTimeoutException();
    }
}

bool dropbox::Ring::AcceptPrev() {
    Socket new_prev(accept(accept_socket_.Get(), nullptr, nullptr));

    if (!new_prev.IsValid()) {
        return false;
    }

    prev_ = std::move(new_prev);
    // This probably isn't necessary, but I'm keeping it for safety's sake
    if (!prev_.SetTimeout({0, 0})) {
        throw Socket::SetTimeoutException();
    }

    return true;
}
