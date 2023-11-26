#pragma once

#include <queue>
#include <thread>
#include <unordered_map>

#include "../common/communication/protocol.hpp"
#include "client.hpp"

namespace dropbox {
class UserInput {
   public:
    UserInput(Client&& client);

    void        Start();
    void        Stop();
    std::string GetQueue();

    Client client_;

   private:
    void HandleCommand(Command command);

    bool reading_;

    std::queue<std::string>                  input_queue_;
    std::string                              input_path_;
    std::unordered_map<std::string, Command> command_map_;
};
}
