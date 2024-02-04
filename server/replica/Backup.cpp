#include "Backup.hpp"
#include "fmt/core.h"
#include <thread>

#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"

dropbox::replica::MainLoopReply dropbox::replica::Backup::MainLoop(std::atomic_bool& shutdown) {
    while (!shutdown) {
        const auto kReceivedCommand = exchange_.exchange_.ReceiveCommand();

        if (!kReceivedCommand.has_value()) {
            if (kReceivedCommand.error() == std::errc::connection_aborted) {
                fmt::println("{}: lost connection to primary", __func__);
                return MainLoopReply::kLostConnectionToPrimary;
            }
            fmt::println("{}: {}", __func__, kReceivedCommand.error().message());
            continue;
        }

        const Command kCommand = kReceivedCommand.value();

        if (kCommand == Command::kUpload) {
            if (!exchange_.exchange_.ReceivePath()) {
            }

            std::istringstream iss(exchange_.exchange_.GetPath().c_str());
            std::string        path;
            std::getline(iss, path, '/');
            std::filesystem::create_directory(path);

            if (!exchange_.exchange_.Receive()) {
            }

            fmt::println("{}: received {} from primary", __func__, exchange_.exchange_.GetPath().c_str());

        } else if (kCommand == Command::kDelete) {
            if (!exchange_.exchange_.ReceivePath()) {
            }

            std::istringstream iss(exchange_.exchange_.GetPath().c_str());
            std::string        path;
            std::getline(iss, path, '/');
            std::filesystem::create_directory(path);

            const std::filesystem::path& file_path = exchange_.exchange_.GetPath();

            if (std::filesystem::exists(file_path)) {
                std::filesystem::remove(file_path);
            }

            fmt::println("{}: deleted {} from primary", __func__, file_path.c_str());
        } else if (kCommand == Command::kSuccess) {
            std::string ip;

            cereal::PortableBinaryInputArchive archive(exchange_.stream_);
            archive(ip);

            client_ips.push_back(ip);

            fmt::println("{}: connected {} in primary", __func__, ip.c_str());
        } else if (kCommand != Command::kExit) {
            fmt::println(stderr, "{}: unexpected command: {}", __func__, kCommand);
        }
    }

    return MainLoopReply::kShutdown;
}

dropbox::replica::Backup::Backup(const sockaddr_in& primary_addr) : primary_addr_(primary_addr), exchange_(Socket()) {
    while (!ConnectToPrimary()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    const auto kKeepAliveResult = exchange_.socket_.SetKeepalive();
    if (!kKeepAliveResult.has_value()) {
        throw Socket::KeepAliveException(kKeepAliveResult.error());
    }

    fmt::println("{}: constructed", __func__);
}
