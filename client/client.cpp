#include "client.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "connections.hpp"
#include "exceptions.hpp"

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
    return write(server_socket_, username_.c_str(), username_.size() + 1) == username_.size() + 1;
}

bool dropbox::Client::Upload(std::filesystem::path &&path) {
    if (!he_.SetCommand(Command::UPLOAD).Send()) {
        return false;
    }

    if (!fe_.SetPath(path.filename()).SendPath()) {
        return false;
    }

    return fe_.SetPath(std::move(path)).Send();
}

bool dropbox::Client::Delete(std::filesystem::path&& file_path) {
    if (!he_.SetCommand(Command::DELETE).Send()) {
        return false;
    }

    if (!fe_.SetPath(file_path.filename()).SendPath()) {
        return false;
    }

    return true;
}

bool dropbox::Client::Download(std::filesystem::path &&file_name) { 
    if (!he_.SetCommand(Command::DOWNLOAD).Send()) {
        return false;
    }

    if (!fe_.SetPath(file_name.filename()).SendPath()) {
        return false;
    }

    return fe_.Receive(); 
}

bool dropbox::Client::GetSyncDir() {
    if (!he_.SetCommand(Command::GET_SYNC_DIR).Send()) {
        return false;
    }

    // quebrando aqui..
    return fe_.Receive();
}

dropbox::Client::~Client() { close(server_socket_); }

