#include "server.hpp"
#include "election/election.hpp"

tl::expected<dropbox::Addr::IdType, std::error_code> dropbox::Server::PerformElection() {
    Election election(ring_, GetId());
    election.StartElection();

    tl::expected<std::optional<Addr::IdType>, bool> result;

    do {
        result = election.ReplyElection();
    } while (result.has_value() && !result->has_value());

    if (result.has_value()) {
        return result->value();
    }

    return tl::make_unexpected(std::make_error_code(static_cast<std::errc>(errno)));
}

bool dropbox::Server::ConnectNext() {
    constexpr auto kMaxDuration = std::chrono::seconds(5);

    size_t next_i   = (addr_index_ + 1) % servers_.size();
    auto   inc_next = [&] { next_i = (next_i + 1) % servers_.size(); };

    while (next_i != addr_index_) {
        const auto kEnd = std::chrono::high_resolution_clock::now() + kMaxDuration;

        while (std::chrono::high_resolution_clock::now() < kEnd && !GetRing().HasNext()) {
            auto &next_addr = servers_[next_i % servers_.size()];
            if (!GetRing().ConnectNext(next_addr)) {
                fmt::println(stderr, "{}: Error when connecting to {}", __func__, next_addr.GetId());
            } else {
                return true;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        inc_next();
        fmt::println("{}: Trying next server {}", __func__, next_i);
    }

    return false;
}

void dropbox::Server::HandleElection(dropbox::Addr::IdType id) {
    if (id == GetId()) {
        replica_.emplace(std::in_place_type<replica::Primary>, GetAddr().GetIp());

        std::get<replica::Primary>(*replica_).AcceptBackupLoop();
        std::get<replica::Primary>(*replica_).MainLoop(shutdown_);
    } else {
        auto addr = servers_[static_cast<size_t>(id)];
        addr.SetPort(replica::Primary::kBackupPort);
        replica_.emplace(std::in_place_type<replica::Backup>, addr.AsAddr());

        std::get<replica::Backup>(*replica_).MainLoop(shutdown_);
    }
}

dropbox::Server::Server(size_t addr_index, std::vector<Addr> &&server_collection, std::atomic_bool &shutdown)
    : addr_index_(addr_index),
      servers_(std::move(server_collection)),
      shutdown_(shutdown),
      ring_(GetAddr()),
      accept_thread_([&](const std::stop_token &stop_token) {
          while (!stop_token.stop_requested()) {
              if (GetRing().AcceptPrev()) {
                  fmt::println("AcceptPrev: new previous");
              } else if (errno != EAGAIN) {
                  perror("AcceptPrev");
              }
          }
      }) {
    const bool kCouldConnectNext = ConnectNext();

    tl::expected<dropbox::Addr::IdType, std::error_code> result;

    if (kCouldConnectNext) {
        // We assume that there was never an error while the previous was trying to connect
        while (!GetRing().HasPrev()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        result = PerformElection();
    } else {
        fmt::println("ElectionThread: No servers found, skipping election");
        result = static_cast<dropbox::Addr::IdType>(addr_index_);
    }

    if (result.has_value()) {
        HandleElection(*result);
    } else {
        fmt::println(stderr, "{}: {}", __func__, result.error().message());
    }
}
