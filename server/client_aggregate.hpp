#pragma once

#include <forward_list>

#include "client_handler.hpp"

namespace dropbox {
class ClientAggregate {
   public:
    ClientAggregate() = default;

    ~ClientAggregate() = default;

    ClientAggregate(const ClientAggregate& other) = delete;

    ClientAggregate(ClientAggregate&& other) = default;

   private:
    static constexpr size_t          kClientLimit = 2U;
    std::forward_list<ClientHandler> list_;
};
}
