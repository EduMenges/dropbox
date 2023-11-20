#include <iostream>
#include <string>

#include "user_input.hpp"

dropbox::UserInput::UserInput() : reading_(false) {}

void dropbox::UserInput::Start() {
   reading_ = true;
   std::thread input_thread_(
      [this]() {
         while (reading_) {
            std::cout << "$ ";
            std::string user_input;
            std::getline(std::cin, user_input);

            input_queue_.push(user_input);

            std::cout << "entrada: " << user_input << '\n';      

            // pattern matching aqui -> upload, download, delete, list_server...

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