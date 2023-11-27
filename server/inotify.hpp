#pragma once

#include <iostream>

#define MAX_EVENTS 1024  /* Maximum number of events to process*/
#define LEN_NAME 16  /* Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME ))

namespace dropbox{
    class Inotify {
        public:
            Inotify(const std::string& username);

            void Start();
            void Stop();
        private:
            bool watching_;
            int fd_, wd_;
            int length_, i_;
            std::string watch_path_;

    };
}