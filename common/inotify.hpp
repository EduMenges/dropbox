#pragma once

#include <iostream>
#include <queue>
#include <vector>

#include "communication/protocol.hpp"

#define MAX_EVENTS 1024                           /* Maximum number of events to process*/
#define LEN_NAME   16                             /* Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE (sizeof(struct inotify_event)) /*size of one event*/
#define BUF_LEN    (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))

namespace dropbox {
class Inotify {
   public:
    Inotify(const std::string& username);

    void Start();
    void Stop();
    void Pause();
    void Resume();
    
    std::vector<std::string> inotify_vector_;
    
   private:
    bool        watching_;
    bool        pause_;
    int         fd_, wd_;
    int         length_, i_;
    std::string watch_path_;
    std::string username_;
};
}
