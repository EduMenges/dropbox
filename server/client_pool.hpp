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

    void Insert(ClientHandler&& handler);

   private:
    int                                              TESTE_OLA;
    std::unordered_map<std::string, ClientComposite> clients_;
};
}
