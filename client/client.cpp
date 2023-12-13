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
      inotify_({}),
      client_sync_(true) {
    if (InvalidSockets(header_socket_, payload_socket_, sync_sc_socket_, sync_cs_socket_)) {
        throw SocketCreation();
    }

    const sockaddr_in kServerAddress = {kFamily, htons(port), {inet_addr(server_ip_address)}};

    MultipleConnect(&kServerAddress, header_socket_, payload_socket_, sync_sc_socket_, sync_cs_socket_);

    he_.SetSocket(header_socket_);
    fe_.SetSocket(payload_socket_);

    sche_.SetSocket(sync_sc_socket_);
    scfe_.SetSocket(sync_sc_socket_);

    cshe_.SetSocket(sync_cs_socket_);
    csfe_.SetSocket(sync_cs_socket_);

    header_stream_.SetSocket(header_socket_);
    payload_stream_.SetSocket(payload_socket_);

    SendUsername();
    GetSyncDir();
}

void dropbox::Client::SendUsername() noexcept(false) {
    cereal::PortableBinaryOutputArchive archive(payload_stream_);
    archive(username_);
    payload_stream_.flush();
}

bool dropbox::Client::Upload(std::filesystem::path &&path) {
    if (!he_.SetCommand(Command::kUpload).Send()) {
        return false;
    }

    if (!fe_.SetPath(SyncDirPath() / path.filename()).SendPath()) {
        return false;
    }

    return fe_.SetPath(std::move(path)).Send();
}

bool dropbox::Client::Delete(std::filesystem::path &&file_path) {
    if (!he_.SetCommand(Command::kDelete).Send()) {
        return false;
    }

    return fe_.SetPath(SyncDirPath() / std::move(file_path).filename()).SendPath();
}

bool dropbox::Client::Download(std::filesystem::path &&file_name) {
    if (!he_.SetCommand(Command::kDownload).Send()) {
        return false;
    }

    if (!fe_.SetPath(SyncDirPath() / file_name.filename()).SendPath()) {
        return false;
    }

    if (!he_.Receive()) {
        return false;
    }

    if (he_.GetCommand() == Command::kError) {
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
        if (!he_.Receive()) {
            return false;
        }

        if (he_.GetCommand() == Command::kSuccess) {
            if (!fe_.ReceivePath()) {
                return false;
            }

            if (!fe_.Receive()) {
                return false;
            }
        } else {
            break;
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
    if (cshe_.SetCommand(Command::kExit).Send()) {
    }
    return he_.SetCommand(Command::kExit).Send();
}

bool dropbox::Client::ListClient() const {
    auto table = ListDirectory(SyncDirPath());

    table.print(std::cout);
    std::cout << '\n';

    return true;
}

bool dropbox::Client::ListServer() {
    if (!he_.SetCommand(Command::kListServer).Send()) {
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
                if (!cshe_.SetCommand(Command::kWriteDir).Send()) {
                }

                if (!csfe_.SetPath(SyncDirPath() / file).SendPath()) {
                }

                if (!csfe_.SetPath(std::move(SyncDirPath() / file)).Send()) {
                }

            } else if (command == "delete") {
                if (!cshe_.SetCommand(Command::kDeleteDir).Send()) {
                }

                if (!csfe_.SetPath(SyncDirPath() / std::move(file)).SendPath()) {
                }
            }
        }
    }
}

void dropbox::Client::ReceiveSyncFromServer() {
    while (client_sync_) {
        if (sche_.Receive()) {
            if (sche_.GetCommand() == Command::kExit) {
                std::cout << "Exiting client...\n";
                return;
            }

            if (sche_.GetCommand() == Command::kWriteDir) {
                inotify_.Pause();

                std::cout << "SERVER -> CLIENT: modified\n";
                if (!scfe_.ReceivePath()) {
                }

                if (!scfe_.Receive()) {
                }

                inotify_.Resume();

            } else if (sche_.GetCommand() == Command::kDeleteDir) {
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
        } else {
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
