#pragma once

#include <netinet/in.h>

#include "ClientPool.hpp"
#include "election/ring.hpp"
#include "networking/addr.hpp"
#include "networking/socket.hpp"
#include "replica/backup.hpp"
#include "replica/primary.hpp"
#include "tl/expected.hpp"
#include "election/election.hpp"
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

    tl::expected<dropbox::Addr::IdType, dropbox::Election::Error> PerformElection();

    dropbox::replica::MainLoopReply HandleElection(Addr::IdType id);

    /**
     * Attempts to connect to the next replica in \p servers_.
     * Will exhaustively try the all of the replicas, until none can be connected.
     * @param offset Where to start the connection in \p servers_ after \p addr_index_.
     * @return
     */
    bool ConnectNext(size_t offset);

    /**
     * Getter for the replica.
     * @pre Assumes that it has already been assigned to.
     * @return The variant with the replica.
     */
    std::variant<replica::Primary, replica::Backup>& GetReplica() { return *replica_; }

    /// Getter for the id.
    [[nodiscard]] constexpr Addr::IdType GetId() const noexcept { return static_cast<Addr::IdType>(addr_index_); }

    /// Getter for the ring.
    [[nodiscard]] Ring& GetRing() noexcept { return ring_; }

   private:
    size_t            addr_index_;  ///< Where, in the given collection, the address of the server is located.
    std::vector<Addr> servers_;     ///< Servers to connect with.

    std::atomic_bool& shutdown_;  ///< Whether to shutdown the server.

    Ring ring_; ///< Ring for use with the election.
    std::jthread accept_thread_; ///< Thead that accepts new previous in the ring.

    std::optional<std::variant<replica::Primary, replica::Backup>> replica_ = std::nullopt; ///< Underlying replica type of the server.

};
}
