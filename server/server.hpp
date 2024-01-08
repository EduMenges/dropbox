#pragma once

#include <netinet/in.h>

#include "ClientPool.hpp"
#include "election/ring.hpp"
#include "networking/addr.hpp"
#include "networking/socket.hpp"
#include "replica/backup.hpp"
#include "replica/primary.hpp"
#include "tl/expected.hpp"
#include <csignal>
#include <variant>

namespace dropbox {
class Server {
   public:
    Server(size_t addr_index, std::vector<Addr>&& server_collection, std::atomic_bool& shutdown);

    /// Server is not copiable due to side effect in destructor.
    Server(const Server& other) = delete;

    Server(Server&& other) = delete;

    ~Server() = default;

    Addr& GetAddr() noexcept { return servers_[addr_index_]; }

    tl::expected<Addr::IdType, std::error_code> PerformElection();

    void HandleElection(Addr::IdType id);

    bool ConnectNext();

    /**
     * Getter for the replica.
     * @pre Assumes that it has already been assigned to.
     * @return The variant with the replica.
     */
    std::variant<replica::Primary, replica::Backup>& GetReplica() { return *replica_; }

    [[nodiscard]] constexpr Addr::IdType GetId() const noexcept { return static_cast<Addr::IdType>(addr_index_); }

    [[nodiscard]] Ring& GetRing() noexcept { return ring_; }

   private:
    size_t            addr_index_;  /// Where, in the given collection, the address of the server is located.
    std::vector<Addr> servers_;

    std::atomic_bool& shutdown_;

    Ring                                                       ring_;
    std::optional<std::variant<replica::Primary, replica::Backup>> replica_ = std::nullopt;

    std::jthread accept_thread_;
};
}
