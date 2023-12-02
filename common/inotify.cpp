#include "inotify.hpp"

#include <signal.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <cstring>

#include <iostream>

#include "utils.hpp"

// The fcntl() function is used so that when we read using the fd descriptor, the thread will not be blocked.
#include <fcntl.h>

dropbox::Inotify::Inotify(const std::string &username) 
    : i_(0),
      watching_(false),
      username_(username) {

    watch_path_ = SyncDirWithPrefix(username_);

    fd_ = inotify_init();

    //if (fcntl(fd_, F_SETFL, O_NONBLOCK) < 0) {
    //    std::cerr << "Error checking for fcntl" << '\n';
    //    return;
    //}

    wd_ = inotify_add_watch(fd_, watch_path_.c_str(), IN_CLOSE_WRITE | IN_DELETE);

    if (wd_ == -1) {
        //std::cerr << "Could not watch: " << watch_path_ << '\n';
    } else {
        std::cerr << "Watching: " << watch_path_ << '\n';
    }
}

void dropbox::Inotify::Start() {
    watching_ = true;
    
    while (watching_) {
        char buffer_[BUF_LEN];

        length_ = read(fd_, buffer_, BUF_LEN);

        if (length_ < 0) {
            std::cerr << "Error: " << strerror(errno) << '\n';
            return;
        }
        
        i_      = 0;  // precisa resetar aqui
        while (i_ < length_) {
            struct inotify_event *event = (struct inotify_event *)&buffer_[i_];
            if (event->len) {
                if (event->mask & IN_CLOSE_WRITE) {
                    if (event->mask & IN_ISDIR) {
                        std::cout << "The directory " << event->name << " was created/modified.\n";
                    } else {
                        std::cout << "The file " << event->name << " was created/modified.\n";
                        inotify_vector_.push_back("write " + std::string(event->name));
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
                        std::cout << "The directory " << event->name << " was deleted.\n";
                    } else {
                        std::cout << "The file " << event->name << " was deleted.\n";
                        inotify_vector_.push_back("delete " + std::string(event->name));
                    }
                }
            }
            i_ += EVENT_SIZE + event->len;
        }
    }
    inotify_rm_watch(fd_, IN_ALL_EVENTS);
    close(fd_);
}

void dropbox::Inotify::Stop() { watching_ = false; }