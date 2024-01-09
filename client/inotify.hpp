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
        Command               command;  ///< What inotify detected.
        std::filesystem::path path;     ///< The affected file.
    };

    explicit Inotify(std::filesystem::path&& watch_path);
    ~Inotify();

    /**
     * Starts the monitoring.
     */
    void Start();

    /**
     * Monitor the @p watch_path_ for changes.
     * @param stop_token Whether to stop.
     */
    void MainLoop(const std::stop_token& stop_token);

    /**
     * Pauses monitoring activities.
     */
    void Pause() noexcept { pause_ = true; }

    /**
     * Resume monitoring activities.
     */
    void Resume() noexcept { pause_ = false; }

    /**
     * @return Whether there are no detected changes in directory.
     */
    bool Empty() const noexcept { return vector_.empty(); }

    /**
     * @return Whether there are detected changes in directory.
     */
    bool HasActions() const noexcept { return !Empty(); }

    auto cbegin() const noexcept { return vector_.cbegin(); }

    auto cend() const noexcept { return vector_.cend(); }

    /**
     * Clear any detected changes from internal collection.
     */
    void Clear() noexcept { vector_.clear(); }

    std::condition_variable cv_;                ///< CV as to changes detected.
    std::mutex              collection_mutex_;  ///< Mutex to protect the internal collection.

   private:
    static constexpr size_t kMaxEvents    = 10U;  ///< Maximum number of events to process at once
    static constexpr size_t kEventSize    = (sizeof(struct inotify_event));
    static constexpr size_t kBufferLength = (kMaxEvents * (kEventSize + PATH_MAX));

    std::vector<Action>   vector_;            ///< Internal collection of changes.
    std::filesystem::path watch_path_;        ///< Where to monitor.
    bool                  watching_ = false;  ///< Whether to keep watching the @p watch_path_.
    std::atomic_bool      pause_    = true;   ///< Whether it is paused.
    int                   fd_;                ///< FD for init.
    int                   wd_;                ///< FD for watch.
};
}
