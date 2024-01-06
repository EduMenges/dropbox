#include "client.hpp"

#include <arpa/inet.h>

#include <iostream>
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
      sc_stream_(sync_sc_socket_),
      cs_stream_(sync_cs_socket_),
      he_(header_socket_),
      fe_(payload_stream_),
      scfe_(sc_stream_),
      csfe_(cs_stream_),
      inotify_(SyncDirPath()),
      client_sync_(true) {
    const sockaddr_in kServerAddress = {kFamily, htons(port), {inet_addr(server_ip_address)}};

    Socket *to_connect[] = {&header_socket_, &payload_socket_, &sync_sc_socket_, &sync_cs_socket_};

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

bool dropbox::Client::Upload(const std::filesystem::path &path) {
    std::lock_guard const kLock(inotify_.collection_mutex_);

    if (!he_.Send(Command::kUpload)) {
        return false;
    }

    const std::filesystem::path kDestPath = SyncDirPath() / path.filename();
    if (!fe_.SetPath(kDestPath).SendPath()) {
        return false;
    }

    if (!fe_.SetPath(path).Send()) {
        return false;
    }

    try {
        inotify_.Pause();
        std::filesystem::copy_file(path, kDestPath);
        inotify_.Resume();
        return true;
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}

bool dropbox::Client::Delete(std::filesystem::path &&file_path) {
    if (!he_.Send(Command::kDelete)) {
        return false;
    }

    const bool kCouldDelete = fe_.SetPath(SyncDirPath() / std::move(file_path).filename()).SendPath();

    if (!kCouldDelete) {
        return false;
    }

    try {
        inotify_.Pause();
        std::filesystem::remove(fe_.GetPath());
        inotify_.Resume();
        return true;
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}

bool dropbox::Client::Download(std::filesystem::path &&file_name) {
    if (!he_.Send(Command::kDownload)) {
        return false;
    }

    if (!fe_.SetPath(SyncDirPath() / file_name).SendPath()) {
        return false;
    }

    fe_.Flush();

    const auto kReceived = he_.Receive();
    if (!kReceived.has_value()) {
        fmt::println(stderr, "Failure when receiving response from server.");
        return false;
    }

    if (kReceived == Command::kError) {
        fmt::println("File was not found on the server.");
        return false;
    }

    return fe_.SetPath(std::move(file_name)).Receive();
}

bool dropbox::Client::GetSyncDir() {
    if (!std::filesystem::exists(SyncDirPath())) {
        std::filesystem::create_directory(SyncDirPath());
    }
    RemoveAllFromDirectory(SyncDirPath());

    do {
        const auto kReceivedCommand = he_.Receive();

        if (kReceivedCommand == Command::kSuccess) {
            if (!fe_.ReceivePath()) {
                return false;
            }

            if (!fe_.Receive()) {
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

dropbox::Client::~Client() { client_sync_ = false; }

bool dropbox::Client::Exit() {
    client_sync_ = false;

    csfe_.SendCommand(Command::kExit);
    csfe_.Flush();

    return he_.Send(Command::kExit);
}

bool dropbox::Client::ListClient() const {
    auto table = ListDirectory(SyncDirPath());

    table.print(std::cout);
    std::cout << '\n';

    return true;
}

bool dropbox::Client::ListServer() {
    if (!he_.Send(Command::kListServer)) {
        return false;
    }

    try {
        cereal::PortableBinaryInputArchive archive(payload_stream_);

        std::string server_table;
        archive(server_table);

        std::cout << server_table << '\n';

        return true;
    } catch (std::exception &e) {
        fmt::println(stderr, "Error when receiving file table: {}", e.what());
        return false;
    }
}

void dropbox::Client::StartInotify(const std::stop_token &stop_token) {
    inotify_.Start();
    inotify_.MainLoop(stop_token);
}

void dropbox::Client::SyncFromClient(std::stop_token stop_token) {
    while (client_sync_) {
        std::unique_lock lk(inotify_.collection_mutex_);
        inotify_.cv_.wait(lk, [&] { return inotify_.HasActions() || stop_token.stop_requested(); });

        if (stop_token.stop_requested()) {
            return;
        }

        for (auto it = inotify_.cbegin(); it != inotify_.cend(); it++) {
            const auto &[command, path] = *it;

            if (command == Command::kUpload) {
                fmt::println("Inotify detected upload: {}", path.c_str());
                csfe_.SendCommand(Command::kUpload);

                if (!csfe_.SetPath(SyncDirPath() / path).SendPath()) {
                }

                if (!csfe_.SetPath(SyncDirPath() / path).Send()) {
                }

            } else if (command == Command::kDelete) {
                fmt::println("Inotify detected delete: {}", path.c_str());
                (csfe_.SendCommand(Command::kDelete));

                if (!csfe_.SetPath(SyncDirPath() / path).SendPath()) {
                }
            }

            csfe_.Flush();
        }

        inotify_.Clear();

        lk.unlock();
        inotify_.cv_.notify_one();
    }
}

void dropbox::Client::SyncFromServer(const std::stop_token &stop_token) {
    /// @todo Error treatment.
    while (!stop_token.stop_requested()) {
        const auto kReceivedCommand = scfe_.ReceiveCommand();

        if (!kReceivedCommand.has_value()) {
            continue;
        }

        const Command kCommand = kReceivedCommand.value();

        if (kCommand == Command::kUpload) {
            if (!scfe_.ReceivePath()) {
            }

            inotify_.Pause();
            if (!scfe_.Receive()) {
            }
            inotify_.Resume();

            fmt::println("{}  was modified from another device", scfe_.GetPath().c_str());

        } else if (kCommand == Command::kDelete) {
            if (!scfe_.ReceivePath()) {
            }

            const std::filesystem::path &file_path = scfe_.GetPath();

            if (std::filesystem::exists(file_path)) {
                inotify_.Pause();
                std::filesystem::remove(file_path);
                inotify_.Resume();
            }

            std::cout << file_path << " was deleted from another device\n";
        } else if (kCommand != Command::kExit) {
            std::cerr << "Unexpected command from server sync: " << kCommand << '\n';
        }
    }
}
