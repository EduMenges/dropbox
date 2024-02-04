#include "BackupHandler.hpp"

dropbox::BackupHandler::BackupHandler(dropbox::Socket&& socket)
    : Sender(std::move(socket)), mutex_(std::make_unique<std::mutex>()) {}

bool dropbox::BackupHandler::Upload(const std::filesystem::path& path) {
    const std::lock_guard kLock(*mutex_);
    return Sender::Upload(path);
}

bool dropbox::BackupHandler::Delete(const std::filesystem::path& path) {
    const std::lock_guard kLock(*mutex_);
    return Sender::Delete(path);
}

bool dropbox::BackupHandler::Ip(std::string ip) {
    const std::lock_guard kLock(*mutex_);
    return Sender::Ip(ip);
}
