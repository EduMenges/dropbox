#pragma once

#include <sys/inotify.h>

#include <climits>
#include <condition_variable>
#include <iostream>
#include <queue>

#include "communication/protocol.hpp"

namespace dropbox {
class Inotify {
   public:
    struct Action {
        Command               command;
        std::filesystem::path path;
    };

    Inotify(std::filesystem::path&& watch_path);
    ~Inotify();

    void Start();
    void Stop();
    void Pause();
    void Resume();

    bool Empty() const noexcept { return queue_.empty(); }

    bool HasActions() const noexcept { return !Empty(); }

    const Action& Front() { return queue_.front(); }

    void Pop() { queue_.pop(); }

    std::condition_variable cv_;
    std::mutex              mutex_;

   private:
    static constexpr size_t kMaxEvents    = 10U;  ///< Maximum number of events to process at once
    static constexpr size_t kEventSize    = (sizeof(struct inotify_event));
    static constexpr size_t kBufferLength = (kMaxEvents * (kEventSize + PATH_MAX));

    std::queue<Action>    queue_;
    std::filesystem::path watch_path_;
    bool                  watching_;
    bool                  pause_;
    int                   fd_, wd_;
    size_t                length_{};
};
}
