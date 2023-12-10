#include "user_input.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>

#include "client.hpp"
#include "communication/protocol.hpp"

dropbox::UserInput::UserInput(Client& client) : client_(client), reading_(false) { }

void dropbox::UserInput::Start() {
    reading_ = true;

    while (reading_) {
        std::cout << "$ ";
        std::cout.flush();

        std::string user_input;
        std::getline(std::cin, user_input);

        input_queue_.push(user_input);

        std::string        input_command;
        std::istringstream iss(user_input);

        iss >> input_command;
        iss >> input_path_;

        if (!CommandFromStr(input_command).has_value()) {
            std::cerr << "Unknown command: " << input_command << '\n';
            continue;
        }

        const Command kCommand = CommandFromStr(input_command).value();
        HandleCommand(kCommand);
    }

}

void dropbox::UserInput::Stop() { reading_ = false; }

std::string dropbox::UserInput::GetQueue() {
    if (input_queue_.empty()) {
        return "";
    }

    std::string front_queue = input_queue_.front();
    input_queue_.pop();

    return front_queue;
}

void dropbox::UserInput::HandleCommand(Command command) {
    switch (command) {
        case Command::kUpload:
            if (!input_path_.empty()) {
                std::filesystem::path path(input_path_);

                if (is_regular_file(path)) {
                    std::cerr << "Result: " << client_.Upload(std::move(path)) << '\n';
                } else {
                    std::cerr << path.filename() << " is not a file\n";
                }
            } else {
                std::cerr << "Missing path\n";
            }
            break;
        case Command::kDownload:
            if (!input_path_.empty()) {
                std::filesystem::path path(input_path_);

                std::cerr << "Result: " << client_.Download(std::move(path)) << '\n';
            } else {
                std::cerr << "Missing path\n";
            }
            break;
        case Command::kDelete:
            if (!input_path_.empty()) {
                std::filesystem::path path(input_path_);

                std::cerr << "Result: " << client_.Delete(std::move(path)) << '\n';
            } else {
                std::cerr << "Missing path.\n";
            }
            break;
        case Command::kListServer:
            client_.ListServer();
            break;
        case Command::kListClient:
            client_.ListClient();
            break;
        case Command::kExit:
            client_.Exit();
            Stop();
            break;
        default:
            std::cerr << "Unexpected command: " << command << '\n';
            break;
    }
}
