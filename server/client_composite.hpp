#pragma once

#include <list>
#include <mutex>

#include "composite_interface.hpp"
#include "client_handler.hpp"

namespace dropbox {
class ClientComposite: public CompositeInterface {
   public:
    ClientComposite() = default;

    ~ClientComposite() override = default;

    ClientComposite(const ClientComposite& other) = delete;

    ClientComposite(ClientComposite&& other) = delete;

    ClientHandler& Insert(ClientHandler&& client) noexcept(false);

    void Remove(int id) override;

   private:
    static constexpr size_t kDeviceLimit = 2U;

    std::mutex               mutex_;
    std::list<ClientHandler> list_;
};
}
