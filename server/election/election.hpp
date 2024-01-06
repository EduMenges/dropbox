#pragma once

#include <optional>

#include "fmt/core.h"
#include "ring.hpp"
#include "tl/expected.hpp"

namespace dropbox {

struct ElectionData {
    enum class Status : uint8_t {
        kUnderway = 0,
        kElected,
    };

    Status       status; // NOLINT
    Addr::IdType id; // NOLINT
};

/**
 * Election uses the ring algorithm.
 */
class Election {
   public:
    Election(Ring &ring, Addr::IdType id) : ring_(ring), id_(id) {}

    bool StartElection();

    /**
     *
     * @return \p error if error in exchanging data; \p nullopt if none was elected yet; \p id of the elected.
     */
    tl::expected<std::optional<Addr::IdType>, bool> ReplyElection();

    [[nodiscard]] constexpr Ring &GetRing() { return ring_; }

   private:
    Ring        &ring_;
    Addr::IdType id_;
    bool         election_started_ = false;
};
}
