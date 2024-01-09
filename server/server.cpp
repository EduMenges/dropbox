#include "server.hpp"
#include "election/election.hpp"

tl::expected<dropbox::Addr::IdType, dropbox::Election::Error> dropbox::Server::PerformElection() {
    Election election(ring_, GetId());

    if (!election.StartElection()) {
        return tl::make_unexpected(Election::Error::kNextBroken);
    }

    tl::expected<std::optional<Addr::IdType>, Election::Error> result;

    do {
        result = election.ReplyElection();
    } while (result.has_value() && !result->has_value());

    if (result.has_value()) {
        return result->value();
    }

    return tl::make_unexpected(result.error());
}

bool dropbox::Server::ConnectNext(size_t offset) {
    /// How much to wait before trying to connect to the next server in @p servers_
    constexpr auto kTimeout = std::chrono::seconds(8);

    size_t next_i   = (addr_index_ + offset) % servers_.size();
    auto   inc_next = [&] { next_i = (next_i + 1) % servers_.size(); };

    while (next_i != addr_index_) {
        const auto kEnd = std::chrono::high_resolution_clock::now() + kTimeout;

        fmt::println("{}: trying server {}", __func__, next_i);

        while (std::chrono::high_resolution_clock::now() < kEnd && !GetRing().HasNext()) {
            auto &next_addr = servers_[next_i % servers_.size()];
            if (!GetRing().ConnectNext(next_addr)) {
                fmt::println(stderr, "{}: error when connecting to {}", __func__, next_addr.GetId());
            } else {
                return true;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        inc_next();
    }

    return false;
}

using dropbox::replica::MainLoopReply;

dropbox::replica::MainLoopReply dropbox::Server::HandleElection(dropbox::Addr::IdType id) {
    if (id == GetId()) {
        replica_.emplace(std::in_place_type<replica::Primary>, GetAddr().GetIp());

        std::get<replica::Primary>(*replica_).AcceptBackupLoop();
        std::get<replica::Primary>(*replica_).MainLoop(shutdown_);

        return MainLoopReply::kShutdown;
    } else {
        Addr addr = servers_[static_cast<size_t>(id)];
        addr.SetPort(replica::Primary::kBackupPort);

        if (replica_.has_value()) {
            auto &backup = std::get<replica::Backup>(*replica_);
            backup.SetPrimaryAddr(addr.AsAddr());
            backup.ConnectToPrimary();
        } else {
            replica_.emplace(std::in_place_type<replica::Backup>, addr.AsAddr());
        }

        return std::get<replica::Backup>(*replica_).MainLoop(shutdown_);
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
      }) {}
