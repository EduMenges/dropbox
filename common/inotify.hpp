#pragma once

#include <sys/inotify.h>

#include <climits>
#include <iostream>
#include <queue>
#include <vector>

#include "communication/protocol.hpp"

namespace dropbox {
class Inotify {
   public:
    Inotify(std::filesystem::path&& watch_path);
    ~Inotify();

    void Start();
    void Stop();
    void Pause();
    void Resume();

    std::vector<std::string> inotify_vector_;

   private:
    static constexpr size_t kMaxEvents    = 20U;                          /* Maximum number of events to process*/
    static constexpr size_t kEventSize    = (sizeof(struct inotify_event)); /*size of one event*/
    static constexpr size_t kBufferLength = (kMaxEvents * (kEventSize + NAME_MAX));

    std::filesystem::path watch_path_;
    bool                  watching_;
    bool                  pause_;
    int                   fd_, wd_;
    size_t                length_{};
};
}
