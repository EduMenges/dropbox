#pragma once

#include <list>
#include <mutex>

#include "client_handler.hpp"

namespace dropbox {
class ClientAggregate {
   public:
    ClientAggregate() = default;

    ~ClientAggregate() = default;

    ClientAggregate(const ClientAggregate& other) = delete;

    ClientAggregate(ClientAggregate&& other) = default;

    bool Insert(ClientHandler&& client);

    void Remove(int id);

   private:
    static constexpr size_t          kClientLimit = 2U;

    std::mutex mutex_;
    std::list<ClientHandler> list_;
};
}
