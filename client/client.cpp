#include "client.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "connections.hpp"
#include "exceptions.hpp"
#include "list_directory.hpp"
#include "../common/constants.hpp"
#include "../common/communication/commands.hpp"

dropbox::Client::Client(std::string &&username, const char *server_ip_address, in_port_t port)
    : username_(std::move(username)),
      header_socket_(socket(kDomain, kType, kProtocol)),
      file_socket_(socket(kDomain, kType, kProtocol)),
      sync_sc_socket_(socket(kDomain, kType, kProtocol)),
      sync_cs_socket_(socket(kDomain, kType, kProtocol)),
      inotify_({}),
      client_sync_(true) {
    if (header_socket_ == kInvalidSocket || file_socket_ == kInvalidSocket || sync_sc_socket_ == kInvalidSocket || sync_cs_socket_ == kInvalidSocket) {
        throw SocketCreation();
    }

    const sockaddr_in kServerAddress = {kFamily, htons(port), {inet_addr(server_ip_address)}};

    if (connect(header_socket_,  reinterpret_cast<const sockaddr *>(&kServerAddress), sizeof(kServerAddress)) == -1 ||
        connect(file_socket_,    reinterpret_cast<const sockaddr *>(&kServerAddress), sizeof(kServerAddress)) == -1 ||
        connect(sync_sc_socket_, reinterpret_cast<const sockaddr *>(&kServerAddress), sizeof(kServerAddress)) == -1 ||
        connect(sync_cs_socket_, reinterpret_cast<const sockaddr *>(&kServerAddress), sizeof(kServerAddress)) == -1) {

        throw Connecting();
    }

    he_.SetSocket(header_socket_);
    fe_.SetSocket(file_socket_);

    sche_.SetSocket(sync_sc_socket_);
    scfe_.SetSocket(sync_sc_socket_);
    
    cshe_.SetSocket(sync_cs_socket_);
    csfe_.SetSocket(sync_cs_socket_);


    if (!SendUsername()) {
        throw Username();
    }

    GetSyncDir();

    // Watching local sync
    inotify_ = Inotify(username_);
    std::thread inotify_client_thread_(
        [this]() {
            inotify_.Start();
        }
    );

    // Thread que manda as atualizações do local para o server
    std::thread file_exchange_thread(
        [this](auto username_, auto client_sync_) {
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
                        if (!cshe_.SetCommand(Command::WRITE_DIR).Send()) { }
                        
                        if (!csfe_.SetPath( SyncDirWithPrefix(username_) / file).SendPath()) { }

                        if (!csfe_.SetPath(std::move(SyncDirWithPrefix(username_) / file)).Send()) { }

                    } else if (command == "delete") {
                        if (!cshe_.SetCommand(Command::DELETE_DIR).Send()) { }

                        if (!csfe_.SetPath(SyncDirWithPrefix(username_) / std::move(file)).SendPath()) {  }       
                    }
                }   
            }
        }, username_, client_sync_
    );
    inotify_client_thread_.detach();
    file_exchange_thread.detach();


}

bool dropbox::Client::SendUsername() {
    if (!he_.SetCommand(Command::USERNAME).Send()) {
        return false;
    }

    // Sending name's length
    size_t username_length = username_.length() + 1;
    if (write(file_socket_, &username_length, sizeof(username_length)) != sizeof(username_length)) {
        perror(__func__);
        return false;
    }

    const auto kBytesSent = write(file_socket_, username_.c_str(), username_length);

    if (kBytesSent != static_cast<ssize_t>(username_length)) {
        perror(__func__);
        return false;
    }

    return true;
}

bool dropbox::Client::Upload(std::filesystem::path &&path) {
    if (!he_.SetCommand(Command::UPLOAD).Send()) {
        return false;
    }

    if (!fe_.SetPath(SyncDirWithPrefix(username_) / path.filename()).SendPath()) {
        return false;
    }

    return fe_.SetPath(std::move(path)).Send();
}

bool dropbox::Client::Delete(std::filesystem::path &&file_path) {
    if (!he_.SetCommand(Command::DELETE).Send()) {
        return false;
    }

    return fe_.SetPath(SyncDirWithPrefix(username_) / std::move(file_path).filename()).SendPath();
}

bool dropbox::Client::Download(std::filesystem::path &&file_name) {
    if (!he_.SetCommand(Command::DOWNLOAD).Send()) {
        return false;
    }

    if (!fe_.SetPath(SyncDirWithPrefix(username_) / file_name.filename()).SendPath()) {
        return false;
    }

    if (!he_.Receive()) {
        return false;
    }

    if (he_.GetCommand() == Command::ERROR) {
        std::cerr << "File not found on the server ";
        return false;
    }

    return fe_.SetPath(std::move(file_name).filename()).Receive();
}

bool dropbox::Client::GetSyncDir() {
    try {
        if (std::filesystem::exists(SyncDirWithPrefix(username_))) {
            std::filesystem::remove_all(SyncDirWithPrefix(username_));
        }
        std::filesystem::create_directory(SyncDirWithPrefix(username_));
    } catch (const std::exception &e) {
        std::cerr << "Error creating directory " << e.what() << '\n';
    }

    do {
        if (!he_.Receive()) {
            return false;
        }

        if (he_.GetCommand() == Command::SUCCESS) {
            if (!fe_.ReceivePath()) {
                return false;
            }

            if (!fe_.Receive()) {
                return false;
            }
        }
    } while (he_.GetCommand() == Command::SUCCESS);

    return true;
}

dropbox::Client::~Client() {
    client_sync_ = false;
    close(header_socket_);
    close(file_socket_);
    close(sync_sc_socket_);
    close(sync_cs_socket_);
}

bool dropbox::Client::Exit() {
    client_sync_ = false;
    return he_.SetCommand(Command::EXIT).Send();
}

bool dropbox::Client::ListClient() const {
    auto table = ListDirectory(SyncDirPath());

    table.print(std::cout);
    std::cout << '\n';

    return true;
}

bool dropbox::Client::ListServer() {
    static thread_local std::array<char, kPacketSize> buffer;

    if (!he_.SetCommand(Command::LIST_SERVER).Send()) {
        return false;
    }

    size_t remaining_size = 0;

    if (read(header_socket_, &remaining_size, sizeof(remaining_size)) == kInvalidRead) {
        perror(__func__);
        return false;
    }

    while (remaining_size != 0) {
        const size_t kBytesToRead = std::min(remaining_size, kPacketSize);

        const ssize_t kBytesRead = read(header_socket_, buffer.data(), kBytesToRead);

        if (kBytesRead == kInvalidRead) {
            perror(__func__);
            return false;
        }

        remaining_size -= kBytesRead;
        std::cout << buffer.data();
    }
    std::cout << '\n';

    return true;
}

bool dropbox::Client::ReceiveSyncFromServer() {
    while(client_sync_){
        if (!sche_.Receive()) {
            return false;
        }
        
        if (sche_.GetCommand() == Command::WRITE_DIR) {
            inotify_.Pause();

            printf("SERVER -> CLIENT: modified\n");
            if (!scfe_.ReceivePath()) {
                return false;
            }

            if (!scfe_.Receive()) {
                return false;
            }

        inotify_.Resume();

            return true;
            
        } else if (sche_.GetCommand() == Command::DELETE_DIR) {
            printf("SERVER -> CLIENT: delete\n");
            if (!scfe_.ReceivePath()) {
                return false;
            }

            const std::filesystem::path& file_path = scfe_.GetPath();

            if (std::filesystem::exists(file_path)) {
                std::filesystem::remove(file_path);
                
                //

                return true;
            }

            return false;
        }

        return true;
    }
}
