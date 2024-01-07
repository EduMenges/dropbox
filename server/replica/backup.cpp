#include "backup.hpp"
#include "fmt/core.h"

void dropbox::replica::Backup::MainLoop(sig_atomic_t& should_stop) {
    while (should_stop != 1) {
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

            fmt::println("{}  was modified from another device", exchange_.exchange_.GetPath().c_str());

        } else if (kCommand == Command::kDelete) {
            if (!exchange_.exchange_.ReceivePath()) {
            }

            const std::filesystem::path& file_path = exchange_.exchange_.GetPath();

            if (std::filesystem::exists(file_path)) {
                std::filesystem::remove(file_path);
            }

            std::cout << file_path << " was deleted from another device\n";
        } else if (kCommand != Command::kExit) {
            std::cerr << "Unexpected command from server sync: " << kCommand << '\n';
        }
    }
}

dropbox::replica::Backup::Backup(const sockaddr_in& primary_addr)
    : primary_addr_(primary_addr), exchange_(Socket(kInvalidSocket)) {
    if (!exchange_.socket_.SetKeepalive()) {
        throw Socket::KeepAliveErr();
    }

    [[maybe_unused]] bool could_connect = ConnectToPrimary();
}
