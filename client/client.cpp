#include "client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"
#include "connections.hpp"
#include "exceptions.hpp"
#include "list_directory.hpp"

dropbox::Client::Client(std::string &&username, const char *server_ip_address, in_port_t port)
    : username_(std::move(username)),
      header_socket_(socket(kDomain, kType, kProtocol)),
      payload_socket_(socket(kDomain, kType, kProtocol)),
      sync_sc_socket_(socket(kDomain, kType, kProtocol)),
      sync_cs_socket_(socket(kDomain, kType, kProtocol)),
      payload_stream_(payload_socket_),
      sc_stream_(sync_sc_socket_),
      cs_stream_(sync_cs_socket_),
      he_(header_socket_),
      fe_(payload_stream_),
      sche_(sync_cs_socket_),
      scfe_(sc_stream_),
      cshe_(sync_cs_socket_),
      csfe_(cs_stream_),
      inotify_({}),
      client_sync_(true) {
    if (InvalidSockets(header_socket_, payload_socket_, sync_sc_socket_, sync_cs_socket_)) {
        throw SocketCreation();
    }

    const sockaddr_in kServerAddress = {kFamily, htons(port), {inet_addr(server_ip_address)}};

    MultipleConnect(&kServerAddress, header_socket_, payload_socket_, sync_sc_socket_, sync_cs_socket_);

    SendUsername();
    GetSyncDir();
}

void dropbox::Client::SendUsername() noexcept(false) {
    cereal::PortableBinaryOutputArchive archive(payload_stream_);
    archive(username_);
    payload_stream_.flush();
}

bool dropbox::Client::Upload(std::filesystem::path &&path) {
    if (!he_.Send(Command::kUpload)) {
        return false;
    }

    if (!fe_.SetPath(SyncDirPath() / path.filename()).SendPath()) {
        return false;
    }

    return fe_.SetPath(std::move(path)).Send();
}

bool dropbox::Client::Delete(std::filesystem::path &&file_path) {
    if (!he_.Send(Command::kDelete)) {
        return false;
    }

    return fe_.SetPath(SyncDirPath() / std::move(file_path).filename()).SendPath();
}

bool dropbox::Client::Download(std::filesystem::path &&file_name) {
    if (!he_.Send(Command::kDownload)) {
        return false;
    }

    if (!fe_.SetPath(SyncDirPath() / file_name.filename()).SendPath()) {
        return false;
    }

    const auto kReceived = he_.Receive();
    if (!kReceived.has_value()) {
        std::cerr << "Failure when receiving response from server.\n";
        return false;
    }

    if (kReceived == Command::kError) {
        std::cerr << "File was not found on the server.\n";
        return false;
    }

    return fe_.SetPath(std::move(file_name).filename()).Receive();
}

bool dropbox::Client::GetSyncDir() {
    try {
        if (std::filesystem::exists(SyncDirPath())) {
            std::filesystem::remove_all(SyncDirPath());
        }
        std::filesystem::create_directory(SyncDirPath());
    } catch (const std::exception &e) {
        std::cerr << "Error creating sync_dir directory: " << e.what() << '\n';
    }

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

dropbox::Client::~Client() {
    client_sync_ = false;
    close(header_socket_);
    close(payload_socket_);
    close(sync_sc_socket_);
    close(sync_cs_socket_);
}

bool dropbox::Client::Exit() {
    client_sync_ = false;
    return cshe_.Send(Command::kExit) && he_.Send(Command::kExit);
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
        std::cerr << "Error when receiving file table: " << e.what() << '\n';
        return false;
    }
}

void dropbox::Client::StartInotify() {
    // Monitora o diretorio
    inotify_ = Inotify(username_);
    inotify_.Start();
}

void dropbox::Client::StartFileExchange() {
    while (client_sync_) {
        if (!inotify_.inotify_vector_.empty()) {
            std::string queue = inotify_.inotify_vector_.front();
            inotify_.inotify_vector_.erase(inotify_.inotify_vector_.begin());
            std::istringstream iss(queue);

            std::string command;
            std::string file;

            iss >> command;
            iss >> file;

            std::cout << "Must att in Server | op:" << command << " in:" << file << '\n';

            if (command == "write") {
                if (!cshe_.Send(Command::kWriteDir)) {
                }

                if (!csfe_.SetPath(SyncDirPath() / file).SendPath()) {
                }

                if (!csfe_.SetPath(SyncDirPath() / file).Send()) {
                }

            } else if (command == "delete") {
                if (!cshe_.Send(Command::kDeleteDir)) {
                }

                if (!csfe_.SetPath(SyncDirPath() / file).SendPath()) {
                }
            }
        }
    }
}

void dropbox::Client::ReceiveSyncFromServer() {
    /// @todo Error treatment.
    while (client_sync_) {
        const auto kReceivedCommand = sche_.Receive();

        if (kReceivedCommand == Command::kExit) {
            std::cout << "Exiting client...\n";
            return;
        }

        if (kReceivedCommand == Command::kWriteDir) {
            inotify_.Pause();

            std::cout << "SERVER -> CLIENT: modified\n";
            if (!scfe_.ReceivePath()) {
            }

            if (!scfe_.Receive()) {
            }

            inotify_.Resume();

        } else if (kReceivedCommand == Command::kDeleteDir) {
            inotify_.Pause();
            std::cout << "SERVER -> CLIENT: delete\n";
            if (!scfe_.ReceivePath()) {
            }

            const std::filesystem::path &file_path = scfe_.GetPath();

            if (std::filesystem::exists(file_path)) {
                std::filesystem::remove(file_path);

                //
            }
            inotify_.Resume();
        }
    }
}
