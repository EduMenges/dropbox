#pragma once

#include <optional>

#include "fmt/core.h"
#include "Ring.hpp"
#include "tl/expected.hpp"

namespace dropbox {

struct ElectionData {
    enum class Status : int8_t {
        kUnderway = -1,  ///< Election has started and is underway
        kElected,        ///< Election finished with a winner
    };

    Status       status;  ///< Status of the election
    Addr::IdType id;      ///< Id of the elected
};

/**
 * Election uses the ring algorithm.
 */
class Election {
   public:
    enum class Error : int8_t {
        kPreviousBroken = -1,  ///< The connection to the previous replica in the ring is broken.
        kNextBroken            ///< The connection to the next replica in the ring is broken.
    };

    /**
     * Constructor.
     * @param ring Ring to pass the messages with.
     * @param id Id of the server with \p this election.
     */
    Election(Ring &ring, Addr::IdType id) : ring_(ring), id_(id) {}

    /**
     * Starts the election.
     * @return Whether it could start the election.
     */
    [[nodiscard]] bool StartElection();

    /**
     * Receives the election message received in @p ring_.prev_ and sends it to @p ring_.next_.
     * @return @p error if error in exchanging data; @p nullopt if none was elected yet; @p id of the elected.
     */
    [[nodiscard]] tl::expected<std::optional<Addr::IdType>, Error> ReplyElection();

   private:
    Ring        &ring_;                      ///< Ring to send and receive data from.
    Addr::IdType id_;                        ///< Id of the server that issued this election.
    bool         election_started_ = false;  ///< Whether the election has started.
};
}

template <>
struct fmt::formatter<dropbox::Election::Error> : formatter<string_view> {
   public:
    constexpr auto format(dropbox::Election::Error error, format_context &ctx) const {
        std::string_view ans;

        switch (error) {
            case dropbox::Election::Error::kPreviousBroken:
                ans = "Previous connection in ring is broken";
                break;
            case dropbox::Election::Error::kNextBroken:
                ans = "Next connection in the ring is broken";
                break;
        }

        return formatter<string_view>::format(ans, ctx);
    }
};
