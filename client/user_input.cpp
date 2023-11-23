#include <iostream>

#include <sstream>

#include "user_input.hpp"
#include "client.hpp"
#include "../common/communication/protocol.hpp"

dropbox::UserInput::UserInput(dropbox::Client client) : reading_(false), client_(client) {
   command_map_ = {
      {"upload", Command::UPLOAD},
      {"download", Command::UPLOAD},
      {"delete", Command::DELETE},
      {"list_server", Command::LIST_SERVER},
      {"list_client", Command::UPLOAD},
      {"get_sync_dir", Command::GET_SYNC_DIR},
      {"exit", Command::EXIT}
   };
}

void dropbox::UserInput::Start() {
   reading_ = true;
   std::thread input_thread_(
      [this]() {
         while (reading_) {
            std::cout << "$ ";
            std::string user_input;
            std::getline(std::cin, user_input);

            input_queue_.push(user_input);

            std::string input_command;
            std::string input_path;
            std::istringstream iss(user_input);
            iss >> input_command;
            iss >> input_path;

            if (command_map_.count(input_command)) {
               HeaderExchange he(client_.GetSocket());
               he.SetCommand(command_map_[input_command]);

               if (he.Send()) {
                  std::cout << "Command sent successfully.\n";
                  if (!input_path.empty()) {
                     FileExchange fe(client_.GetSocket());
                     fe.SetPath(input_path);

                     if (fe.SendPath()) {
                        std::cout << "Path sent successfully.\n";
                     } else {
                        std::cerr << "Failed to send path.\n";
                     }
                  }
               } else {
                  std::cerr << "Failed to send command.\n";
               }
            } else {
               std::cout << "Unknown command: " << input_command << '\n';
            }

            if (user_input == "exit") {
               Stop();
            }
         }
      }
   );
   input_thread_.join();
}

void dropbox::UserInput::Stop() {
   reading_ = false;
}

std::string dropbox::UserInput::GetQueue() {
   if (input_queue_.empty()) {
      return "";
   }

   std::string front_queue = input_queue_.front();
   input_queue_.pop();

   return front_queue;
}