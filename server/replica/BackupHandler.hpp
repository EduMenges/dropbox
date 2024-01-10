#pragma once

#include <mutex>

#include "networking/Socket.hpp"
#include "composite/Sender.hpp"
#include "communication/protocol.hpp"
#include "networking/SocketStream.hpp"

namespace dropbox {
/// To be used in conjunction with @ref dropbox::Primary
class BackupHandler : public composite::Sender {
   public:
    /**
     * Constructor.
     * @param socket Socket to communicate with the backup replica.
     */
    explicit BackupHandler(Socket&& socket);

    BackupHandler(const BackupHandler& other) = delete;

    BackupHandler(BackupHandler&& other) = default;

    ~BackupHandler() override = default;

    /**
     * Uploads a file to the replica.
     * @param path Path of the file.
     * @return Success of the operation.
     */
    bool Upload(const std::filesystem::path& path) override;

    /**
     * Issues a delete of a file to the replica.
     * @param path Path of the file.
     * @return Success of the operation.
     */
    bool Delete(const std::filesystem::path& path) override;

   private:
    /// Mutex to protected the methods and prevent concurrent access to the socket.
    std::unique_ptr<std::mutex> mutex_;
};
}
