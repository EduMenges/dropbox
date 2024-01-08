#include "ClientHandler.hpp"

#include <filesystem>
#include <utility>
#include <vector>

#define CEREAL_THREAD_SAFE 1  // NOLINT
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "list_directory.hpp"
#include "fmt/core.h"

dropbox::ClientHandler::ClientHandler(CompositeInterface* composite, Socket&& payload_socket, Socket&& client_sync,
                                      Socket&& server_sync, SocketStream&& payload_stream)
    : composite_(composite),
      payload_socket_(std::move(payload_socket)),
      client_sync_(std::move(client_sync)),
      server_sync_composite_(std::move(server_sync)),
      payload_stream_(std::move(payload_stream)),
      cs_stream_(client_sync_),
      fe_(payload_stream_),
      csfe_(cs_stream_) {
    payload_stream_.SetSocket(payload_socket_);

    CreateUserFolder();

    GetSyncDir();
}

dropbox::ClientHandler::ClientHandler(ClientHandler&& other) noexcept
    : composite_(std::exchange(other.composite_, nullptr)),
      payload_socket_(std::move(other.payload_socket_)),
      client_sync_(std::move(other.client_sync_)),
      server_sync_composite_(std::move(other.server_sync_composite_)),
      payload_stream_(std::move(other.payload_stream_)),
      cs_stream_(std::move(other.cs_stream_)),
      fe_(payload_stream_),
      csfe_(cs_stream_) {}

void dropbox::ClientHandler::MainLoop() {
    bool    receiving = true;
    uint8_t attempts  = kAttemptAmount;

    while (receiving) {
        const auto kReceived = fe_.ReceiveCommand();

        if (kReceived.has_value()) {
            attempts = kAttemptAmount;
            fmt::println("{}::{} ordered {}", GetUsername(), GetId(), *kReceived);
            switch (kReceived.value()) {
                case Command::kUpload:
                    Upload();
                    break;
                case Command::kDownload:
                    Download();
                    break;
                case Command::kDelete:
                    Delete();
                    break;
                case Command::kExit:
                    receiving = false;
                    break;
                case Command::kListServer:
                    ListServer();
                    break;
                default:
                    fmt::println(stderr, "Unexpected command: ", *kReceived);
                    break;
            }

            payload_stream_.flush();
        } else {
            attempts -= 1;
            if (attempts == 0) {
                receiving = false;
                fmt::println("{}::{} timed out", GetUsername(), GetId());
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}

bool dropbox::ClientHandler::Upload() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    if (!fe_.Receive()) {
        return false;
    }

    return GetComposite()->BroadcastUpload(GetId(), fe_.GetPath());
}

bool dropbox::ClientHandler::Delete() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    if (std::filesystem::exists(fe_.GetPath())) {
        std::filesystem::remove(fe_.GetPath());

        return GetComposite()->BroadcastDelete(GetId(), fe_.GetPath());
    }

    return false;
}

bool dropbox::ClientHandler::Download() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    if (!std::filesystem::exists(fe_.GetPath())) {
        fe_.SendCommand(Command::kError);
        return false;
    }

    fe_.SendCommand(Command::kSuccess);

    return fe_.Send();
}

bool dropbox::ClientHandler::GetSyncDir() {
    const std::filesystem::path        kSyncPath = SyncDirPath();
    std::vector<std::filesystem::path> file_names;

    std::ranges::for_each(std::filesystem::directory_iterator(kSyncPath),
                          [&](const auto& entry) { file_names.push_back(entry.path().filename()); });

    for (const auto& file_name : file_names) {
        fe_.SendCommand(Command::kSuccess);

        if (!fe_.SetPath(kSyncPath / file_name.filename()).SendPath()) {
            return false;
        }

        if (!fe_.Send()) {
            return false;
        }
    }

    fe_.SendCommand(Command::kExit);
    return true;
}

dropbox::ClientHandler::~ClientHandler() {
    if (GetId() != kInvalidId) {
        fmt::println("{}::{} disconnected", GetUsername(), GetId());
    }
}

bool dropbox::ClientHandler::ListServer() noexcept {
    std::string const kStrTable = ListDirectory(SyncDirPath()).str();

    try {
        cereal::PortableBinaryOutputArchive archive(payload_stream_);
        archive(kStrTable);
        return true;
    } catch (std::exception& e) {
        return false;
    }
}

bool dropbox::ClientHandler::SyncDelete(const std::filesystem::path& path) {
    return server_sync_composite_.Delete(path);
}

bool dropbox::ClientHandler::SyncUpload(const std::filesystem::path& path) {
    return server_sync_composite_.Upload(path);
}

void dropbox::ClientHandler::SyncFromClient(std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
        const auto kReceivedCommand = csfe_.ReceiveCommand();

        if (!kReceivedCommand.has_value()) {
            continue;
        }

        const Command kCommand = *kReceivedCommand;

        if (kCommand == Command::kUpload) {
            if (!csfe_.ReceivePath()) {
            }

            if (!csfe_.Receive()) {
            }

            fmt::println("{}::{} modified {}", GetUsername(), GetId(), csfe_.GetPath().c_str());
            GetComposite()->BroadcastUpload(GetId(), csfe_.GetPath());
        } else if (kCommand == Command::kDelete) {
            if (!csfe_.ReceivePath()) {
            }

            std::filesystem::remove(csfe_.GetPath());

            fmt::println("{}::{} deleted {}", GetUsername(), GetId(), csfe_.GetPath().c_str());
            GetComposite()->BroadcastDelete(GetId(), csfe_.GetPath());
        }
    }
}
