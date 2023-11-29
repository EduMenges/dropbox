#pragma once

#include <string>
#include <unordered_map>

#include "client_composition.hpp"

namespace dropbox {
class ClientPool {
   public:
    ClientPool()                        = default;
    ~ClientPool()                       = default;
    ClientPool(const ClientPool& other) = delete;
    ClientPool(ClientPool&& other)      = default;

   private:
    std::unordered_map<std::string, ClientAggregate> clients_;
};
}
