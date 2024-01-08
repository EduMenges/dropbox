#pragma once

#include <sys/inotify.h>

#include <climits>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <stop_token>

#include "communication/protocol.hpp"

namespace dropbox {
class Inotify {
   public:
    struct Action {
        Command               command;
        std::filesystem::path path;
    };

    explicit Inotify(std::filesystem::path&& watch_path);
    ~Inotify();

    void Start();
    void MainLoop(const std::stop_token& stop_token);

    void Pause();
    void Resume();

    bool Empty() const noexcept { return vector_.empty(); }

    bool HasActions() const noexcept { return !Empty(); }

    auto cbegin() const noexcept { return vector_.cbegin(); }

    auto cend() const noexcept { return vector_.cend(); }

    void Clear() noexcept { vector_.clear(); }

    std::condition_variable cv_;
    std::mutex              collection_mutex_;

   private:
    static constexpr size_t kMaxEvents    = 10U;  ///< Maximum number of events to process at once
    static constexpr size_t kEventSize    = (sizeof(struct inotify_event));
    static constexpr size_t kBufferLength = (kMaxEvents * (kEventSize + PATH_MAX));

    std::vector<Action>   vector_;
    std::filesystem::path watch_path_;
    bool                  watching_;
    bool                  pause_;
    int                   fd_, wd_;
};
}
