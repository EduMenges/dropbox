#pragma once

#include <netinet/in.h>

#include "ClientPool.hpp"
#include "election/Ring.hpp"
#include "networking/Addr.hpp"
#include "networking/Socket.hpp"
#include "replica/Backup.hpp"
#include "replica/Primary.hpp"
#include "tl/expected.hpp"
#include "election/Election.hpp"
#include <csignal>
#include <variant>

namespace dropbox {
class Server {
   public:
    /**
     * Constructor.
     * @param addr_index Where in @p server_collection the info for @p this is.
     * @param server_collection Collection with info regarding connecting to other servers.
     * @param shutdown Whether to shutdown.
     */
    Server(size_t addr_index, std::vector<Addr>&& server_collection, std::atomic_bool& shutdown);

    /// Server is not copiable due to side effect in destructor.
    Server(const Server& other) = delete;

    Server(Server&& other) = delete;

    ~Server() = default;

    /* constexpr */ Addr& GetAddr() noexcept { return servers_[addr_index_]; }

    tl::expected<dropbox::Addr::IdType, dropbox::Election::Error> PerformElection();

    /**
     * Handles the result of the election by instantiating @p replica_ according to the result.
     * @param id Id of the elected server.
     * @return Status on how the main loop of the server exited.
     */
    dropbox::replica::MainLoopReply HandleElection(Addr::IdType id);

    /**
     * Attempts to connect to the next replica in @p servers_.
     * Will exhaustively try the all of the replicas, until none can be connected.
     * @param offset Where to start the connection in @p servers_ after @p addr_index_.
     * @return Whether a connection to the @p next in @p ring_ could be done.
     */
    bool ConnectNext(size_t offset);

    /// Getter for the id.
    [[nodiscard]] constexpr Addr::IdType GetId() const noexcept { return static_cast<Addr::IdType>(addr_index_); }

    /// Getter for the @p ring_.
    [[nodiscard]] Ring& GetRing() noexcept { return ring_; }

    void SetServers() {
        if (replica_.has_value() && std::holds_alternative<replica::Primary>(replica_.value())) {
            std::get<replica::Primary>(replica_.value()).SetServers(servers_);
        }
    }

   private:
    size_t            addr_index_;  ///< Where, in the given collection, the address of the server is located.
    std::vector<Addr> servers_;     ///< Servers to connect with.

    std::atomic_bool& shutdown_;  ///< Whether to shutdown the server.

    Ring         ring_;           ///< Ring for use with the election.
    std::jthread accept_thread_;  ///< Thead that accepts new previous in the ring.

    /// Underlying replica type of the server.
    std::optional<std::variant<replica::Primary, replica::Backup>> replica_ = std::nullopt;
};
}
