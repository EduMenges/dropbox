#include "client.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "connections.hpp"
#include "exceptions.hpp"
#include "list_directory.hpp"

dropbox::Client::Client(std::string &&username, const char *server_ip_address, in_port_t port)
    : username_(std::move(username)), server_socket_(socket(kDomain, kType, kProtocol)) {
    if (this->server_socket_ == -1) {
        throw SocketCreation();
    }

    const sockaddr_in kServerAddress = {kFamily, htons(port), inet_addr(server_ip_address)};

    if (connect(server_socket_, reinterpret_cast<const sockaddr *>(&kServerAddress), sizeof(kServerAddress)) == -1) {
        throw Connecting();
    }

    if (!SendUsername()) {
        throw Username();
    }

    he_.SetSocket(server_socket_);
    fe_.SetSocket(server_socket_);
    de_.SetSocket(server_socket_);
}

int dropbox::Client::GetSocket() const { return server_socket_; }

bool dropbox::Client::SendUsername() {
    auto kBytesSent = write(server_socket_, username_.data(), username_.size());

    if (kBytesSent == -1) {
        perror(__func__);
        return false;
    }

    return kBytesSent == username_.size();
}

bool dropbox::Client::Upload(std::filesystem::path &&path) {
    if (!he_.SetCommand(Command::UPLOAD).Send()) {
        return false;
    }

    if (!fe_.SetPath(SyncDirPath() / path.filename()).SendPath()) {
        return false;
    }

    return fe_.SetPath(std::move(path)).Send();
}

bool dropbox::Client::Download(std::filesystem::path &&file_name) { return false; }

dropbox::Client::~Client() { close(server_socket_); }

bool dropbox::Client::GetSyncDir() {
    /// @todo This.
    return false;
}

bool dropbox::Client::Exit() { return he_.SetCommand(Command::EXIT).Send(); }

bool dropbox::Client::ListClient() {
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

    if (read(server_socket_, &remaining_size, sizeof(remaining_size)) == kInvalidRead) {
        perror(__func__);
        return false;
    }

    while (remaining_size != 0) {
        const size_t kBytesToRead = std::min(remaining_size, kPacketSize);

        const ssize_t kBytesRead = read(server_socket_, buffer.data(), kBytesToRead);

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
