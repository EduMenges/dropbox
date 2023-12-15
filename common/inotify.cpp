#include "inotify.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "utils.hpp"

dropbox::Inotify::Inotify(std::filesystem::path &&watch_path)
    : watch_path_(std::move(watch_path)),
      watching_(false),
      pause_(false),
      fd_(inotify_init()),
      wd_(inotify_add_watch(fd_, watch_path_.c_str(), IN_CLOSE_WRITE | IN_DELETE)) {
    SetNonblocking(fd_);

    if (wd_ == -1) {
        std::cerr << "Could not watch: " << watch_path_ << '\n';
    } else {
        std::cerr << "Watching: " << watch_path_ << '\n';
    }
}

void dropbox::Inotify::Start() {
    static thread_local std::array<uint8_t, kBufferLength> buffer;

    while(watching_) {
        length_ = read(fd_, buffer.data(), kBufferLength);

        size_t i = 0;
        while (i < length_ && !pause_) {
            auto *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

            if (event->len != 0U) {
                if ((event->mask & IN_CLOSE_WRITE) != 0U) {
                    if ((event->mask & IN_ISDIR) != 0U) {
                        std::cout << "The directory " << event->name << " was created/modified.\n";
                    } else {
                        std::cout << "The file " << event->name << " was created/modified.\n";
                        inotify_vector_.push_back("write " + std::string(event->name));
                    }
                } else if ((event->mask & IN_DELETE) != 0U) {
                    if ((event->mask & IN_ISDIR) != 0U) {
                        std::cout << "The directory " << event->name << " was deleted.\n";
                    } else {
                        std::cout << "The file " << event->name << " was deleted.\n";
                        inotify_vector_.push_back("delete " + std::string(event->name));
                    }
                }
            }
            i += kEventSize + event->len;
        }
    }
}

void dropbox::Inotify::Stop() { watching_ = false; }

void dropbox::Inotify::Pause() { pause_ = true; }

void dropbox::Inotify::Resume() { pause_ = false; }

dropbox::Inotify::~Inotify() {
    inotify_rm_watch(fd_, IN_ALL_EVENTS);
    close(fd_);
}
