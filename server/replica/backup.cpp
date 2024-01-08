#include "backup.hpp"
#include "fmt/core.h"

void dropbox::replica::Backup::MainLoop(std::atomic_bool& shutdown) {
    while (!shutdown) {
        const auto kReceivedCommand = exchange_.exchange_.ReceiveCommand();

        if (!kReceivedCommand.has_value()) {
            continue;
        }

        const Command kCommand = kReceivedCommand.value();

        if (kCommand == Command::kUpload) {
            if (!exchange_.exchange_.ReceivePath()) {
            }

            if (!exchange_.exchange_.Receive()) {
            }

            fmt::println("{}: {} was modified from another device", __func__, exchange_.exchange_.GetPath().c_str());

        } else if (kCommand == Command::kDelete) {
            if (!exchange_.exchange_.ReceivePath()) {
            }

            const std::filesystem::path& file_path = exchange_.exchange_.GetPath();

            if (std::filesystem::exists(file_path)) {
                std::filesystem::remove(file_path);
            }

            fmt::println( "{}: {} was deleted from another device", __func__, file_path.c_str());
        } else if (kCommand != Command::kExit) {
            fmt::println(stderr, "{}: unexpected command: {}", __func__, kCommand);
        }
    }
}

dropbox::replica::Backup::Backup(const sockaddr_in& primary_addr)
    : primary_addr_(primary_addr), exchange_(Socket()) {
    [[maybe_unused]] bool could_connect = ConnectToPrimary();

//    const auto kKeepAliveResult = exchange_.socket_.SetKeepalive();
//    if (!kKeepAliveResult.has_value()) {
//        throw Socket::KeepAliveException(kKeepAliveResult.error());
//    }
}
