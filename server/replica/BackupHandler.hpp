#pragma once

#include <mutex>

#include "networking/Socket.hpp"
#include "composite/Sender.hpp"
#include "communication/protocol.hpp"
#include "networking/SocketStream.hpp"

namespace dropbox {
class BackupHandler : public composite::Sender {
   public:
    explicit BackupHandler(Socket&& socket);

    BackupHandler(const BackupHandler& other) = delete;

    BackupHandler(BackupHandler&& other) = default;

    ~BackupHandler() override = default;

    bool Upload(const std::filesystem::path& path) override;

    bool Delete(const std::filesystem::path& path) override;

   private:
    std::unique_ptr<std::mutex> mutex_;
};
}
