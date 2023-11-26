#include "user_input.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>

#include "client.hpp"
#include "communication/protocol.hpp"

dropbox::UserInput::UserInput(dropbox::Client&& client) : reading_(false), client_(std::move(client)) {
    command_map_ = {{"upload", Command::UPLOAD},
                    {"download", Command::DOWNLOAD},
                    {"delete", Command::DELETE},
                    {"list_server", Command::LIST_SERVER},
                    {"list_client", Command::LIST_CLIENT},
                    {"get_sync_dir", Command::GET_SYNC_DIR},
                    {"exit", Command::EXIT}};
}

void dropbox::UserInput::Start() {
    reading_ = true;
    std::thread input_thread_([this]() {  // NOLINT
        while (reading_) {
            std::cout << "$ ";

            std::string user_input;
            std::getline(std::cin, user_input);

            input_queue_.push(user_input);

            std::string        input_command;
            std::string        input_path;
            std::istringstream iss(user_input);
            iss >> input_command;
            iss >> input_path;

            Command command = command_map_[input_command];

            if (!command_map_.contains(input_command)) {
                std::cerr << "Unknown command: " << input_command << '\n';
                continue;
            }

            HeaderExchange he(client_.GetSocket(), command);

            switch (command) {
                case Command::UPLOAD:
                    if (!input_path.empty()) {
                        std::filesystem::path path(input_path);

                        if (is_regular_file(path)) {
                            std::cerr << "Result: " << client_.Upload(std::move(path)) << '\n';
                        } else {
                            std::cerr << path.filename() << " is not a file\n";
                        }
                    } else {
                        std::cerr << "Missing path\n";
                    }
                    break;
                case Command::DOWNLOAD:
                    if (!input_path.empty()) {
                        std::filesystem::path path(input_path);

                        std::cerr << "Result: " << client_.Download(std::move(path)) << '\n';
                    } else {
                        std::cerr << "Missing path\n";
                    }
                    break;
                case Command::DELETE:
                    if (!input_path.empty()) {
                        std::filesystem::path path(input_path);

                        std::cerr << "Result: " << client_.Delete(std::move(path)) << '\n';
                    } else {
                        std::cerr << "Missing path.\n";
                    }
                    break;
                case Command::LIST_SERVER:
                case Command::LIST_CLIENT:
                case Command::GET_SYNC_DIR:
                    std::cerr << "Result: " << client_.GetSyncDir() << '\n';
                    break;
                case Command::EXIT:
                    if (input_path.empty()) {
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
                    std::cerr << "Unknown command: " << input_command << '\n';
                    break;
            }

            if (user_input == "exit") {
                Stop();
            }
        }
    });

    input_thread_.join();
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
