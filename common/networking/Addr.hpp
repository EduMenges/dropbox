#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <string>

#include "connections.hpp"

namespace dropbox {
class Addr {
   public:
    using IdType = uint8_t;

    /**
     * Constructor.
     * @pre \p ip port in host byte-order.
     * @param ip Ip in IPv4 dot format.
     * @param port Port.
     */
    Addr(IdType id, std::string &&ip, in_port_t port)
        : id_(id), ip_(std::move(ip)), port_(port), addr_({kFamily, htons(port_), {inet_addr(ip_.c_str())}, {0}}) {}

    Addr(Addr&& other) = default;

    Addr(const Addr& other) = default;

    ~Addr() = default;

    [[nodiscard]] constexpr const sockaddr_in &AsAddr() const noexcept { return addr_; }

    [[nodiscard]] constexpr IdType GetId() const noexcept { return id_; }

    [[nodiscard]] constexpr const std::string &GetIp() const noexcept { return ip_; }

    void SetPort(in_port_t port) {
        port_          = port;
        addr_.sin_port = htons(port_);
    }

   private:
    IdType      id_; ///< Id of the address (if it represents a server).
    std::string ip_; ///< Ip.
    in_port_t   port_; ///< Port.
    sockaddr_in addr_; ///< Address for interfacing with POSIX functions.
};
}
