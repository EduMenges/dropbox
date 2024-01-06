#include "election.hpp"

bool dropbox::Election::StartElection()  {
    election_started_ = true;

    ElectionData message{ElectionData::Status::kUnderway, id_};
    return write(ring_.next_socket_, &message, sizeof(message)) == sizeof(ElectionData);
}

tl::expected<std::optional<dropbox::Addr::IdType>, bool> dropbox::Election::ReplyElection()  {
    ElectionData data; // NOLINT

    auto receive_election = [&] { return read(ring_.prev_socket_, &data, sizeof(data)); };
    auto send_election    = [&] { return write(ring_.next_socket_, &data, sizeof(data)); };

    if (receive_election() != sizeof(data)) {
        return tl::unexpected(false);
    }

    switch (data.status) {
        case ElectionData::Status::kUnderway:
            if (data.id < id_) {
                if (election_started_) {
                    fmt::println("{}: {} was rejected due to election already ongoing", __func__, data.id);
                    return std::nullopt;
                }

                election_started_ = true;
                data.id           = id_;
                fmt::println("{}: {} was substituted over my id", __func__, data.id);
            } else if (data.id == id_) {
                data.status = ElectionData::Status::kElected;
                fmt::println("{}: {} won the election", __func__, data.id);
            } else {
                fmt::println("{}: {} was forwarded", __func__, data.id);
            }

            if (send_election() != sizeof(data)) {
                return tl::unexpected(false);
            }

            return std::nullopt;
        case ElectionData::Status::kElected:
            election_started_ = false;

            /// We only send the election if \c id wasn't elected
            if (data.id != id_) {
                if (send_election() != sizeof(data)) {
                    return tl::unexpected(false);
                }
                fmt::println("{}: {} was elected", __func__, data.id);
            }

            return data.id;
    }
}
