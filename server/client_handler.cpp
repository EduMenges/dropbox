#include "client_handler.hpp"

#include <dirent.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <utility>
#include <vector>

#define CEREAL_THREAD_SAFE 1  // NOLINT
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "exceptions.hpp"
#include "list_directory.hpp"

dropbox::ClientHandler::ClientHandler(int header_socket, int file_socket, int sync_sc_socket, int sync_cs_socket)
    : header_socket_(header_socket),
      file_socket_(file_socket),
      sync_sc_socket_(sync_sc_socket),
      sync_cs_socket_(sync_cs_socket),
      he_(header_socket),
      fe_(file_socket),
      sche_(sync_sc_socket),
      scfe_(sync_sc_socket),
      cshe_(sync_cs_socket),
      csfe_(sync_cs_socket),
      inotify_({}),
      server_sync_(true) {
    header_stream_.SetSocket(header_socket_);
    file_stream_.SetSocket(file_socket_);

    ReceiveUsername();

    // Cria diretorio no servidor
    CreateUserFolder();

    // Puxa o diretorio para a maquina do client
    ReceiveGetSyncDir();

    he_.Flush();
    fe_.Flush();
}

dropbox::ClientHandler::ClientHandler(ClientHandler&& other) noexcept
    : header_socket_(std::exchange(other.header_socket_, -1)),
      file_socket_(std::exchange(other.file_socket_, -1)),
      sync_sc_socket_(std::exchange(other.sync_sc_socket_, -1)),
      sync_cs_socket_(std::exchange(other.sync_cs_socket_, -1)),
      username_(std::move(other.username_)),
      composite_(std::exchange(other.composite_, nullptr)),
      he_(std::move(other.he_)),
      fe_(std::move(other.fe_)),
      sche_(std::move(other.sche_)),
      scfe_(std::move(other.scfe_)),
      cshe_(std::move(other.cshe_)),
      csfe_(std::move(other.csfe_)),
      inotify_({}),
      server_sync_(std::exchange(other.server_sync_, false)),
      header_stream_(std::move(other.header_stream_)),
      file_stream_(std::move(other.file_stream_)) {}

void dropbox::ClientHandler::ReceiveUsername() {
    cereal::PortableBinaryInputArchive archive(file_stream_);
    archive(username_);
}

void dropbox::ClientHandler::MainLoop() {
    bool    receiving = true;
    uint8_t attempts  = kAttemptAmount;

    while (receiving) {
        if (he_.Receive()) {
            attempts = kAttemptAmount;
            std::cout << username_ << " ordered " << he_.GetCommand() << std::endl;  // NOLINT
            switch (he_.GetCommand()) {
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
                    std::cerr << "Unexpected command received: " << he_.GetCommand() << '\n';
                    break;
            }
        } else {
            attempts -= 1;
            if (attempts == 0) {
                receiving = false;
                std::cerr << "Client " << username_ << " with id " << GetId() << " timed out" << '\n';
            } else {
                std::cerr << "Could not get response from " << username_ << ' ' << GetId() << ", sleeping\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
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
        std::cerr << "Error: File does not exist - " << fe_.GetPath() << '\n';

        return he_.SetCommand(Command::kError).Send();
    }

    const bool kCouldSendSuccess = he_.SetCommand(Command::kSuccess).Send();
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
        if (!he_.SetCommand(Command::kSuccess).Send()) {
            return false;
        }

        if (!fe_.SetPath(kSyncPath / file_name.filename()).SendPath()) {
            return false;
        }

        if (!fe_.Send()) {
            return false;
        }
    }

    return he_.SetCommand(Command::kExit).Send();
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

bool dropbox::ClientHandler::ListServer() const {
    std::string const kStrTable = ListDirectory(SyncDirPath()).str();

    const size_t kTableSize = kStrTable.size() + 1;

    if (write(header_socket_, &kTableSize, sizeof(kTableSize)) == kInvalidWrite) {
        perror(__func__);  // NOLINT
        return false;
    }

    size_t total_sent = 0;
    while (total_sent != kTableSize) {
        const size_t kBytesToSend = std::min(kPacketSize, kTableSize - total_sent);

        const ssize_t kBytesSent = write(header_socket_, kStrTable.c_str() + total_sent, kBytesToSend);
        if (kBytesSent < static_cast<ssize_t>(kBytesToSend)) {
            perror(__func__);
            return false;
        }

        total_sent += kTableSize;
    }

    return true;
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
                if (!sche_.SetCommand(Command::kWriteDir).Send()) {
                }

                if (!scfe_.SetPath(SyncDirPath() / file).SendPath()) {
                }

                if (!scfe_.SetPath(SyncDirPath() / file).Send()) {
                }

            } else if (command == "delete") {
                if (!sche_.SetCommand(Command::kDeleteDir).Send()) {
                }

                if (!scfe_.SetPath(SyncDirPath() / std::move(file)).SendPath()) {
                }
            }

            sche_.Flush();
            scfe_.Flush();
        }
    }
}

void dropbox::ClientHandler::ReceiveSyncFromClient() {
    server_sync_ = true;

    // fcntl(sync_cs_socket_, F_SETFL, O_NONBLOCK);

    while (server_sync_) {
        if (cshe_.Receive()) {
            if (cshe_.GetCommand() == Command::kExit) {
                printf("server exiting...\n");
                if (sche_.SetCommand(Command::kExit).Send()) {
                }
                return;
            }

            if (cshe_.GetCommand() == Command::kWriteDir) {
                inotify_.Pause();

                std::cout << "CLIENT -> SERVER: modified\n";
                if (!csfe_.ReceivePath()) {
                }

                if (!csfe_.Receive()) {
                }

                inotify_.Resume();

            } else if (cshe_.GetCommand() == Command::kDeleteDir) {
                inotify_.Pause();
                std::cout << "CLIENT -> SERVER: delete\n";
                if (!csfe_.ReceivePath()) {
                }

                const std::filesystem::path& file_path = csfe_.GetPath();

                if (std::filesystem::exists(file_path)) {
                    std::filesystem::remove(file_path);

                    //
                }
                inotify_.Resume();
            }
        }
    }
}
