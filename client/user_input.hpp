#pragma once

#include <thread>

#include <queue>

#include <unordered_map>

#include "client.hpp"
#include "../common/communication/protocol.hpp"

namespace dropbox{
class UserInput{
   public:
    UserInput(Client client);

    void Start();
    void Stop();
    std::string GetQueue();

    Client client_;

   private:
    std::thread input_thread_;
    bool reading_;

    std::queue<std::string> input_queue_;
    std::unordered_map<std::string, Command> command_map_;
};
}
