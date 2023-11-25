#include <iostream>

#include <sstream>

#include "user_input.hpp"
#include "client.hpp"
#include "../common/communication/protocol.hpp"

dropbox::UserInput::UserInput(dropbox::Client client) : reading_(false), client_(client) {
   command_map_ = {
      {"upload", Command::UPLOAD},
      {"download", Command::DOWNLOAD},
      {"delete", Command::DELETE},
      {"list_server", Command::LIST_SERVER},
      {"list_client", Command::LIST_CLIENT},
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

            Command command = command_map_[input_command];

            switch (command) {
               case Command::UPLOAD:
               case Command::DOWNLOAD:
               case Command::DELETE:
                  if (!input_path.empty()) {
                     HeaderExchange he(client_.GetSocket());
                     he.SetCommand(command);
                     if (he.Send()) {
                        FileExchange fe(client_.GetSocket());
                        fe.SetPath(input_path);
                        if (fe.SendPath()) {
                           std::cout << "sent: " << input_command << " " << input_path << "\n";
                        } else {
                           std::cerr << "Failed to send path.\n";
                        }
                     } else {
                        std::cerr << "Failed to send command.\n";
                     }
                  } else {
                     std::cerr << "Missing path.\n";
                  }
                  break;
               case Command::LIST_SERVER:
               case Command::LIST_CLIENT:
               case Command::GET_SYNC_DIR:
               case Command::EXIT:
                  if (input_path.empty()) {
                     HeaderExchange he(client_.GetSocket());
                     he.SetCommand(command);
                     if (he.Send()) {
                        std::cout << "sent: " << input_command << "\n";
                     } else {
                        std::cerr << "Failed to send command.\n";
                     }
                  } else {
                     std::cerr << "Invalid args: " << input_path << "\n";
                  }
                  break;
               default:
                  std::cout << "Unknown command: " << input_command << '\n';
                  break;
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