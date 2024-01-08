#include "user_input.hpp"

#include <filesystem>
#include <sstream>

#include "client.hpp"
#include "communication/protocol.hpp"
#include "fmt/core.h"

dropbox::UserInput::UserInput(Client& client) : client_(client), reading_(false) {}

void dropbox::UserInput::Start() {
    reading_ = true;

    while (reading_) {
        fmt::print("$ ");

        std::string user_input;
        std::getline(std::cin, user_input);

        input_queue_.push(user_input);

        std::string        input_command;
        std::istringstream iss(user_input);

        iss >> input_command;
        iss >> input_path_;

        const std::optional<Command>& command = CommandFromStr(input_command);

        if (!command.has_value()) {
            fmt::println(stderr, "Unknown command: \"{}\"", input_command);
            continue;
        }

        HandleCommand(*command);
    }
}

void dropbox::UserInput::Stop() { reading_ = false; }

void dropbox::UserInput::HandleCommand(Command command) {
    auto do_and_report = [&](std::function<bool(dropbox::Client&, std::filesystem::path&&)> method,
                             std::filesystem::path&&                                        path) {
        if (method(client_, std::move(path))) {
            fmt::println("Success");
        } else {
            fmt::println(stderr, "Failure");
        }
    };

    switch (command) {
        case Command::kUpload:
            if (!input_path_.empty()) {
                std::filesystem::path path(input_path_);

                if (is_regular_file(path)) {
                    do_and_report(&Client::Upload, std::move(path));
                } else {
                    fmt::println(stderr, "{} is not a file", path.filename().c_str());
                }
            } else {
                fmt::println(stderr, "Missing path");
            }
            break;
        case Command::kDownload:
            if (!input_path_.empty()) {
                std::filesystem::path path(input_path_);

                do_and_report(&Client::Download, std::move(path));

            } else {
                fmt::println(stderr, "Missing path");
            }
            break;
        case Command::kDelete:
            if (!input_path_.empty()) {
                std::filesystem::path path(input_path_);

                do_and_report(&Client::Delete, std::move(path));
            } else {
                fmt::println(stderr, "Missing path");
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
            fmt::println(stderr, "Unexpected command: {}", command);
            break;
    }
    client_.Flush();
}
