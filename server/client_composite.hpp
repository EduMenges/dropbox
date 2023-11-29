#pragma once

#include <list>
#include <mutex>

#include "client_handler.hpp"

namespace dropbox {
class ClientComposite {
   public:
    ClientComposite() = default;

    ~ClientComposite() = default;

    ClientComposite(const ClientComposite& other) = delete;

    ClientComposite(ClientComposite&& other) = default;

    ClientHandler& Insert(ClientHandler&& client) noexcept(false);

    void Remove(int id);

   private:
    static constexpr size_t kClientLimit = 2U;

    std::mutex               mutex_;
    std::list<ClientHandler> list_;
};
}
