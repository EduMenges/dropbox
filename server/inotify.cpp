#include "inotify.hpp"

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/inotify.h>

// The fcntl() function is used so that when we read using the fd descriptor, the thread will not be blocked.
#include <fcntl.h>

dropbox::Inotify::Inotify() : i_(0), watching_(false){
    watch_path_ = ".";

    fd_ = inotify_init();


    if (fcntl(fd_, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error checking for fcntl" << '\n';
        exit(2);
    }

    wd_ = inotify_add_watch(fd_ , watch_path_.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);

    if(wd_ == -1){
        std::cerr << "Could not watch: " << watch_path_ << '\n';
    } else {
        std::cerr << "Watching: " << watch_path_ << '\n';
    } 
}

void dropbox::Inotify::Start() {
    watching_ = true;
    while(watching_){
        char buffer_[BUF_LEN];
        length_ = read(fd_, buffer_, BUF_LEN);
        i_ = 0; // precisa resetar aqui
        while(i_ < length_){
            struct inotify_event *event = (struct inotify_event *) &buffer_[i_];
            if(event->len){
                if (event->mask & IN_CREATE) {
                    if (event->mask & IN_ISDIR) {
                        std::cout << "The directory " << event->name << " was created.\n";
                    } else {
                        std::cout << "The file " << event->name << " was created.\n";
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
                        std::cout << "The directory " << event->name << " was deleted.\n";
                    } else {
                        std::cout << "The file " << event->name << " was deleted.\n";
                    }
                } else if (event->mask & IN_MODIFY) {
                    if (event->mask & IN_ISDIR) {
                        std::cout << "The directory " << event->name << " was modified.\n";
                    } else {
                        std::cout << "The file " << event->name << " was modified.\n";
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