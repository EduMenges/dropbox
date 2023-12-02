#pragma once

#include <queue>
#include <thread>

#include "communication/protocol.hpp"
#include "client.hpp"

namespace dropbox {
/// Command line interface to use the client with.
class UserInput {
   public:
    /**
     * Wraps the client with the CLI.
     * @param client Client to communicate with the server.
     * @pre \p client is working.
     */
    UserInput(Client& client);

    /// Starts the user input handling.
    void Start();

    /// Stops the user input handling..
    void Stop();

    /// Não sei o que isso faz, tem que ver com o Arthur.
    std::string GetQueue();

   private:
    /**
     * Calls the methods corresponding to the command.
     * @param command Command to call the methods.
     */
    void HandleCommand(Command command);

    Client& client_;   ///< Client instance to call the methods from.
    bool   reading_;  ///< Whether to retrieve the user input.

    std::queue<std::string> input_queue_; ///< Não sei, pergunta pro Arthur.
    std::string             input_path_; ///< Não sei, pergunta pro Arthur.
};
}
