#include "client_handler.hpp"

#include <unistd.h>

#include <filesystem>
#include <utility>
#include <vector>

#define CEREAL_THREAD_SAFE 1  // NOLINT
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "exceptions.hpp"
#include "list_directory.hpp"

dropbox::ClientHandler::ClientHandler(CompositeInterface* composite, int header_socket, SocketStream&& payload_stream,
                                      int sync_sc_socket, int sync_cs_socket)
    : composite_(composite),
      header_socket_(header_socket),
      sync_sc_socket_(sync_sc_socket),
      sync_cs_socket_(sync_cs_socket),
      payload_stream_(std::move(payload_stream)),
      sc_stream_(sync_sc_socket_),
      cs_stream_(sync_cs_socket_),
      he_(header_socket),
      fe_(payload_stream_),
      sche_(sync_sc_socket),
      scfe_(sc_stream_),
      cshe_(sync_cs_socket),
      csfe_(cs_stream_),
      server_sync_(true) {
    // Cria diretorio no servidor
    CreateUserFolder();

    // Puxa o diretorio para a maquina do client
    GetSyncDir();
}

dropbox::ClientHandler::ClientHandler(ClientHandler&& other) noexcept
    : composite_(std::exchange(other.composite_, nullptr)),
      header_socket_(std::exchange(other.header_socket_, -1)),
      sync_sc_socket_(std::exchange(other.sync_sc_socket_, -1)),
      sync_cs_socket_(std::exchange(other.sync_cs_socket_, -1)),
      payload_stream_(std::move(other.payload_stream_)),
      sc_stream_(std::move(other.cs_stream_)),
      cs_stream_(std::move(other.cs_stream_)),
      he_(std::move(other.he_)),
      fe_(payload_stream_),
      sche_(std::move(other.sche_)),
      scfe_(sc_stream_),
      cshe_(std::move(other.cshe_)),
      csfe_(cs_stream_),
      server_sync_(std::exchange(other.server_sync_, false)) {}

void dropbox::ClientHandler::MainLoop() {
    bool    receiving = true;
    uint8_t attempts  = kAttemptAmount;

    while (receiving) {
        const auto kReceived = he_.Receive();

        if (kReceived.has_value()) {
            attempts = kAttemptAmount;
            std::cout << GetUsername() << " ordered " << kReceived.value() << std::endl;  // NOLINT
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
                    server_sync_ = false;
                    receiving    = false;
                    break;
                case Command::kListServer:
                    ListServer();
                    break;
                default:
                    std::cerr << "Unexpected command received: " << kReceived.value() << '\n';
                    break;
            }
        } else {
            attempts -= 1;
            if (attempts == 0) {
                receiving = false;
                std::cerr << "Client " << GetUsername() << " with id " << GetId() << " timed out" << '\n';
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        payload_stream_.flush();
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
        std::cerr << GetUsername() << "::Download(): File does not exist - " << fe_.GetPath() << '\n';

        return he_.Send(Command::kError);
    }

    const bool kCouldSendSuccess = he_.Send(Command::kSuccess);
    if (!kCouldSendSuccess) {
        return false;
    }

    return fe_.Send();
}

bool dropbox::ClientHandler::GetSyncDir() {
    const std::filesystem::path        kSyncPath = SyncDirPath();
    std::vector<std::filesystem::path> file_names;

    try {
        std::ranges::for_each(std::filesystem::directory_iterator(kSyncPath),
                              [&](const auto& entry) { file_names.push_back(entry.path().filename()); });
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return false;
    }

    for (const auto& file_name : file_names) {
        if (!he_.Send(Command::kSuccess)) {
            return false;
        }

        if (!fe_.SetPath(kSyncPath / file_name.filename()).SendPath()) {
            return false;
        }

        if (!fe_.Send()) {
            return false;
        }
    }

    fe_.Flush();
    return he_.Send(Command::kExit);
}

void dropbox::ClientHandler::CreateUserFolder() const {
    try {
        std::filesystem::create_directory(SyncDirPath());
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory: " << e.what() << '\n';
    }
}

dropbox::ClientHandler::~ClientHandler() {
    if (GetId() != kInvalidId) {
        std::cout << GetUsername() << ' ' << GetId() << " disconnected" << std::endl;  // NOLINT
    }

    server_sync_ = false;

    close(header_socket_);
    close(payload_stream_.GetSocket());
    close(sync_sc_socket_);
    close(sync_cs_socket_);
}

bool dropbox::ClientHandler::ListServer() noexcept {
    std::string const kStrTable = ListDirectory(SyncDirPath()).str();

    try {
        cereal::PortableBinaryOutputArchive archive(payload_stream_);
        archive(kStrTable);
        return true;
    } catch (std::exception& e) {
        std::cerr << "Error when sending file table: " << e.what() << '\n';
        return false;
    }
}

bool dropbox::ClientHandler::SyncDelete(const std::filesystem::path& path) {
    sc_stream_ << static_cast<int8_t>(Command::kDelete);
    return scfe_.SetPath(path).SendPath();
}

bool dropbox::ClientHandler::SyncUpload(const std::filesystem::path& path) {
    sc_stream_ << static_cast<int8_t>(Command::kUpload);

    if (!scfe_.SetPath(path).SendPath()) {
        return false;
    }

    return scfe_.Send();
}

void dropbox::ClientHandler::SyncFromClient() {
    server_sync_ = true;

    while (server_sync_) {
        const optional<Command> kReceivedCommand = cshe_.Receive();

        if (!kReceivedCommand.has_value()) {
            continue;
        }

        const Command kCommand = kReceivedCommand.value();

        if (kCommand == Command::kUpload) {
            if (!csfe_.ReceivePath()) {
            }

            if (!csfe_.Receive()) {
            }

            std::cout << GetUsername() << " id " << GetId() << " modified " << csfe_.GetPath() << '\n';
            GetComposite()->BroadcastUpload(GetId(), csfe_.GetPath());
        } else if (kCommand == Command::kDelete) {
            if (!csfe_.ReceivePath()) {
            }

            std::cout << GetUsername() << " id " << GetId() << " deleted " << csfe_.GetPath() << '\n';
            GetComposite()->BroadcastDelete(GetId(), csfe_.GetPath());
        }
    }
}
