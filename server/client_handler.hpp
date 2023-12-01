#pragma once

#include <iostream>
#include <thread>

#include "communication/protocol.hpp"
#include "composite_interface.hpp"
#include "utils.hpp"

namespace dropbox {

class ClientHandler {
   public:
    ClientHandler(int header_socket, int file_socket);

    /// Clients handlers are not copiable due to side effect in socket closing.
    ClientHandler(const ClientHandler& other) = delete;

    ClientHandler(ClientHandler&& other) noexcept ;

    ~ClientHandler();

    void MainLoop();

    void CreateUserFolder();

    bool ReceiveUsername();

    [[nodiscard]] const std::string& GetUsername() const noexcept { return username_; };

    inline void SetComposite(CompositeInterface* composite) noexcept { composite_ = composite; };

    [[nodiscard]] inline CompositeInterface* GetComposite() const noexcept { return composite_; }

    /// RECEIVES an upload from the client.
    bool ReceiveUpload();

    bool ReceiveDownload();

    bool ReceiveDelete();

    bool ReceiveGetSyncDir();

    bool ListServer() const;

    [[nodiscard]] inline int GetId() const noexcept { return header_socket_; }

    inline bool operator==(const ClientHandler& other) const noexcept { return GetId() == other.GetId(); }

    inline bool operator==(int socket) const noexcept { return socket == GetId(); }

    [[nodiscard]] inline std::filesystem::path SyncDirPath() const { return SyncDirWithPrefix(username_); }

   private:
    /// How many attempts remain until a client is disconnected.
    static constexpr uint8_t kAttemptAmount = 5;

    int         header_socket_;
    int         file_socket_;
    std::string username_;

    CompositeInterface* composite_;

    HeaderExchange    he_;
    FileExchange      fe_;

    std::thread inotify_server_thread;

    bool sync_{};
};
}
