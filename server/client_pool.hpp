#pragma once

#include <string>
#include <unordered_map>

#include "client_composite.hpp"

namespace dropbox {
class ClientPool {
   public:
    ClientPool()                        = default;

    ~ClientPool()                       = default;

    ClientPool(const ClientPool& other) = delete;

    ClientPool(ClientPool&& other)      = default;

    ClientHandler& Insert(ClientHandler&& handler) noexcept(false);

   private:
    std::unordered_map<std::string, ClientComposite> clients_;
};
}
