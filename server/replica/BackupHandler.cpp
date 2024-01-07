#include "BackupHandler.hpp"

dropbox::BackupHandler::BackupHandler(dropbox::Socket&& socket) : Sender(std::move(socket)) {}

bool dropbox::BackupHandler::Upload(const std::filesystem::path& path) {
    const std::lock_guard kLock(*mutex_);
    return Sender::Upload(path);
}

bool dropbox::BackupHandler::Delete(const std::filesystem::path& path) {
    const std::lock_guard kLock(*mutex_);
    return Sender::Delete(path);
}
