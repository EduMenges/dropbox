#include "client.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
#include <mutex>
#include <utility>

#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "connections.hpp"
#include "exceptions.hpp"
#include "fmt/core.h"
#include "list_directory.hpp"

dropbox::Client::Client(std::string &&username, const char *server_ip_address, in_port_t port)
    : username_(std::move(username)),
      payload_stream_(payload_socket_),
      client_stream_(client_sync_),
      server_stream_(server_sync_),
      payload_fe_(payload_stream_),
      client_fe_(client_stream_),
      server_fe_(server_stream_),
      inotify_(SyncDirPath()) {
    const sockaddr_in kServerAddress = {kFamily, htons(port), {inet_addr(server_ip_address)}, {0}};

    Socket *to_connect[] = {&payload_socket_, &client_sync_, &server_sync_};  // NOLINT

    for (Socket *socket : to_connect) {
        if (!socket->Connect(kServerAddress)) {
            throw Connecting();
        }
    }

    SendUsername();
    GetSyncDir();
}

void dropbox::Client::SendUsername() noexcept(false) {
    cereal::PortableBinaryOutputArchive archive(payload_stream_);
    archive(GetUsername());
    payload_stream_.flush();
}

bool dropbox::Client::Upload(std::filesystem::path &&path) {
    std::lock_guard const kLock(inotify_.collection_mutex_);

    payload_fe_.SendCommand(Command::kUpload);

    const std::filesystem::path kDestPath = SyncDirPath() / path.filename();
    if (!payload_fe_.SetPath(kDestPath).SendPath()) {
        return false;
    }

    if (!payload_fe_.SetPath(path).Send()) {
        return false;
    }

    try {
        inotify_.Pause();
        std::filesystem::copy_file(path, kDestPath);
        inotify_.Resume();
        return true;
    } catch (std::exception &e) {
        fmt::println(stderr, "{}: {}", __func__, e.what());
        return false;
    }
}

bool dropbox::Client::Delete(std::filesystem::path &&file_path) {
    payload_fe_.SendCommand(Command::kDelete);

    const bool kCouldDelete = payload_fe_.SetPath(SyncDirPath() / std::move(file_path).filename()).SendPath();

    if (!kCouldDelete) {
        return false;
    }

    try {
        inotify_.Pause();
        std::filesystem::remove(payload_fe_.GetPath());
        inotify_.Resume();
        return true;
    } catch (std::exception &e) {
        fmt::println(stderr, "{}: {}", __func__, e.what());
        return false;
    }
}

bool dropbox::Client::Download(std::filesystem::path &&file_name) {
    payload_fe_.SendCommand(Command::kDownload);

    if (!payload_fe_.SetPath(SyncDirPath() / file_name).SendPath()) {
        return false;
    }

    payload_fe_.Flush();

    const auto kReceived = payload_fe_.ReceiveCommand();
    if (!kReceived.has_value()) {
        fmt::println(stderr, "{}: {}", __func__, kReceived.error().message());
        return false;
    }

    if (kReceived == Command::kError) {
        fmt::println(stderr, "{}: file was not found on the server", __func__);
        return false;
    }

    return payload_fe_.SetPath(std::move(file_name)).Receive();
}

bool dropbox::Client::GetSyncDir() {
    if (!std::filesystem::exists(SyncDirPath())) {
        std::filesystem::create_directory(SyncDirPath());
    }
    RemoveAllFromDirectory(SyncDirPath());

    do {
        const auto kReceivedCommand = payload_fe_.ReceiveCommand();

        if (kReceivedCommand == Command::kSuccess) {
            if (!payload_fe_.ReceivePath()) {
                return false;
            }

            if (!payload_fe_.Receive()) {
                return false;
            }
        } else if (kReceivedCommand == Command::kExit) {
            break;
        } else {
            return false;
        }
    } while (true);

    return true;
}

void dropbox::Client::Exit() {
    client_fe_.SendCommand(Command::kExit);
    client_fe_.Flush();

    payload_fe_.SendCommand(Command::kExit);
}

bool dropbox::Client::ListClient() const {
    auto table = ListDirectory(SyncDirPath());

    table.print(std::cout);
    std::cout << '\n';

    return true;
}

bool dropbox::Client::ListServer() {
    payload_fe_.SendCommand(Command::kListServer);

    try {
        cereal::PortableBinaryInputArchive archive(payload_stream_);

        std::string server_table;
        archive(server_table);

        fmt::println("{}", server_table);
        ;

        return true;
    } catch (std::exception &e) {
        fmt::println(stderr, "{}: error when receiving file table {}", __func__, e.what());
        return false;
    }
}

void dropbox::Client::StartInotify(const std::stop_token &stop_token) {
    inotify_.Start();
    inotify_.MainLoop(stop_token);
}

void dropbox::Client::SyncFromClient(std::stop_token stop_token) {
    while (true) {
        std::unique_lock lk(inotify_.collection_mutex_);
        inotify_.cv_.wait(lk, [&] { return inotify_.HasActions() || stop_token.stop_requested(); });

        if (stop_token.stop_requested()) {
            return;
        }

        for (auto it = inotify_.cbegin(); it != inotify_.cend(); it++) {
            const auto &[command, path] = *it;

            if (command == Command::kUpload) {
                fmt::println("Inotify detected upload: {}", path.c_str());
                client_fe_.SendCommand(Command::kUpload);

                if (!client_fe_.SetPath(SyncDirPath() / path).SendPath()) {
                }

                if (!client_fe_.SetPath(SyncDirPath() / path).Send()) {
                }

            } else if (command == Command::kDelete) {
                fmt::println("Inotify detected delete: {}", path.c_str());
                (client_fe_.SendCommand(Command::kDelete));

                if (!client_fe_.SetPath(SyncDirPath() / path).SendPath()) {
                }
            }

            client_fe_.Flush();
        }

        inotify_.Clear();

        lk.unlock();
        inotify_.cv_.notify_one();
    }
}

void dropbox::Client::SyncFromServer(const std::stop_token &stop_token) {
    while (!stop_token.stop_requested()) {
        const auto kReceivedCommand = server_fe_.ReceiveCommand();

        if (!kReceivedCommand.has_value()) {
            if (kReceivedCommand.error() != std::errc::connection_aborted) {
                fmt::println(stderr, "{}: {}", __func__, kReceivedCommand.error().message());
            }
            continue;
        }

        const Command kCommand = *kReceivedCommand;

        if (kCommand == Command::kUpload) {
            if (!server_fe_.ReceivePath()) {
            }

            inotify_.Pause();
            if (!server_fe_.Receive()) {
            }
            inotify_.Resume();

            fmt::println("{}: {} was modified from another device", __func__, server_fe_.GetPath().filename().c_str());

        } else if (kCommand == Command::kDelete) {
            if (!server_fe_.ReceivePath()) {
            }

            const std::filesystem::path &file_path = server_fe_.GetPath();

            if (std::filesystem::exists(file_path)) {
                inotify_.Pause();
                std::filesystem::remove(file_path);
                inotify_.Resume();
            }

            fmt::println("{}: {} was deleted from another device", __func__, server_fe_.GetPath().filename().c_str());
        } else if (kCommand != Command::kExit) {
            fmt::println(stderr, "{} unexpected command {}", __func__, kCommand);
        }
    }
}
