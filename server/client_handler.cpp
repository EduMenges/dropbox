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

dropbox::ClientHandler::ClientHandler(int header_socket, int payload_socket, int sync_sc_socket, int sync_cs_socket)
    : header_socket_(header_socket),
      file_socket_(payload_socket),
      sync_sc_socket_(sync_sc_socket),
      sync_cs_socket_(sync_cs_socket),
      payload_stream_(file_socket_),
      sc_stream_(sync_sc_socket_),
      cs_stream_(sync_cs_socket_),
      he_(header_socket),
      fe_(payload_stream_),
      sche_(sync_sc_socket),
      scfe_(sc_stream_),
      cshe_(sync_cs_socket),
      csfe_(cs_stream_),
      inotify_({}),
      server_sync_(true) {
    ReceiveUsername();
    fe_.Flush();

    // Cria diretorio no servidor
    CreateUserFolder();

    // Puxa o diretorio para a maquina do client
    ReceiveGetSyncDir();
}

dropbox::ClientHandler::ClientHandler(ClientHandler&& other) noexcept
    : username_(std::move(other.username_)),
      composite_(std::exchange(other.composite_, nullptr)),
      header_socket_(std::exchange(other.header_socket_, -1)),
      file_socket_(std::exchange(other.file_socket_, -1)),
      sync_sc_socket_(std::exchange(other.sync_sc_socket_, -1)),
      sync_cs_socket_(std::exchange(other.sync_cs_socket_, -1)),
      payload_stream_(std::move(other.payload_stream_)),
      sc_stream_(std::move(other.cs_stream_)),
      cs_stream_(std::move(other.cs_stream_)),
      he_(std::move(other.he_)),
      fe_(std::move(other.fe_)),
      sche_(std::move(other.sche_)),
      scfe_(std::move(other.scfe_)),
      cshe_(std::move(other.cshe_)),
      csfe_(std::move(other.csfe_)),
      inotify_({}),
      server_sync_(std::exchange(other.server_sync_, false)) {}

void dropbox::ClientHandler::ReceiveUsername() {
    cereal::PortableBinaryInputArchive archive(payload_stream_);
    archive(username_);
}

void dropbox::ClientHandler::MainLoop() {
    bool    receiving = true;
    uint8_t attempts  = kAttemptAmount;

    while (receiving) {
        const auto kReceived = he_.Receive();

        if (kReceived.has_value()) {
            attempts = kAttemptAmount;
            std::cout << username_ << " ordered " << kReceived.value() << std::endl;  // NOLINT
            switch (kReceived.value()) {
                case Command::kUpload:
                    ReceiveUpload();
                    break;
                case Command::kDownload:
                    ReceiveDownload();
                    break;
                case Command::kDelete:
                    ReceiveDelete();
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
                std::cerr << "Client " << username_ << " with id " << GetId() << " timed out" << '\n';
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        payload_stream_.flush();
    }
}

bool dropbox::ClientHandler::ReceiveUpload() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    return fe_.Receive();
}

bool dropbox::ClientHandler::ReceiveDelete() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    const std::filesystem::path& file_path = fe_.GetPath();

    if (std::filesystem::exists(file_path)) {
        std::filesystem::remove(file_path);
        return true;
    }

    return false;
}

bool dropbox::ClientHandler::ReceiveDownload() {
    if (!fe_.ReceivePath()) {
        return false;
    }

    if (!std::filesystem::exists(fe_.GetPath())) {
        std::cerr << username_ << "::ReceiveDownload(): File does not exist - " << fe_.GetPath() << '\n';

        return he_.Send(Command::kError);
    }

    const bool kCouldSendSuccess = he_.Send(Command::kSuccess);
    if (!kCouldSendSuccess) {
        return false;
    }

    return fe_.Send();
}

bool dropbox::ClientHandler::ReceiveGetSyncDir() {
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
        std::cout << username_ << ' ' << GetId() << " disconnected" << std::endl;  // NOLINT
    }

    inotify_.Stop();

    server_sync_ = false;

    close(header_socket_);
    close(file_socket_);
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

void dropbox::ClientHandler::StartInotify() {
    // Monitora o diretorio
    inotify_ = Inotify(username_);
    inotify_.Start();
}

void dropbox::ClientHandler::StartFileExchange() {
    // Troca os arquivos sv -> client
    server_sync_ = true;
    while (server_sync_) {
        if (!inotify_.inotify_vector_.empty()) {
            std::string queue = inotify_.inotify_vector_.front();
            inotify_.inotify_vector_.erase(inotify_.inotify_vector_.begin());
            std::istringstream iss(queue);

            std::string command;
            std::string file;

            iss >> command;
            iss >> file;

            std::cout << "Must att in Client | op:" << command << " in:" << file << '\n';

            if (command == "write") {
                if (!sche_.Send(Command::kWriteDir)) {
                }

                if (!scfe_.SetPath(SyncDirPath() / file).SendPath()) {
                }

                if (!scfe_.SetPath(SyncDirPath() / file).Send()) {
                }

            } else if (command == "delete") {
                if (!sche_.Send(Command::kDeleteDir)) {
                }

                if (!scfe_.SetPath(SyncDirPath() / std::move(file)).SendPath()) {
                }
            }

            scfe_.Flush();
        }
    }
}

void dropbox::ClientHandler::ReceiveSyncFromClient() {
    /// @todo Error treatment.
    server_sync_ = true;

    while (server_sync_) {
        const auto kReceived = cshe_.Receive();
        if (kReceived.has_value()) {
            if (kReceived.value() == Command::kExit) {
                if (sche_.Send(Command::kExit)) {
                }
                return;
            }

            if (kReceived.value() == Command::kWriteDir) {
                inotify_.Pause();

                std::cout << "CLIENT -> SERVER: modified\n";
                if (!csfe_.ReceivePath()) {
                }

                if (!csfe_.Receive()) {
                }

                inotify_.Resume();

            } else if (kReceived.value() == Command::kDeleteDir) {
                inotify_.Pause();
                std::cout << "CLIENT -> SERVER: delete\n";
                if (!csfe_.ReceivePath()) {
                }

                const std::filesystem::path& file_path = csfe_.GetPath();

                if (std::filesystem::exists(file_path)) {
                    std::filesystem::remove(file_path);
                }
                inotify_.Resume();
            }
        }
    }
}
