#include "inotify.hpp"

#include <unistd.h>

#include <iostream>

#include "utils.hpp"

dropbox::Inotify::Inotify(std::filesystem::path &&watch_path)
    : watch_path_(std::move(watch_path)), watching_(false), pause_(false), fd_(inotify_init()), wd_(-1) {
    //    SetNonblocking(fd_);
    if (fd_ == -1) {
        throw InotifyCreate();
    }
}

void dropbox::Inotify::Start() {
    static thread_local std::array<uint8_t, kBufferLength> buffer;

    wd_ = inotify_add_watch(fd_, watch_path_.c_str(), IN_CLOSE_WRITE | IN_DELETE | IN_MOVE | IN_MODIFY);

    if (wd_ == -1) {
        throw InotifyWatch(watch_path_);
    }

    watching_ = true;

    while (watching_) {
        length_ = read(fd_, buffer.data(), kBufferLength);

        size_t i = 0;
        while (i < length_ && !pause_) {
            auto *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

            if (event->len != 0U) {
                if ((event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO)) != 0U) {
                    if ((event->mask & IN_ISDIR) != 0U) {
                        std::cout << "The directory " << event->name << " was created/modified.\n";
                    } else {
                        std::cout << "The file " << event->name << " was created/modified.\n";
                        inotify_vector_.push_back("write " + std::string(event->name));
                    }
                } else if ((event->mask & (IN_DELETE | IN_MOVED_FROM)) != 0U) {
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
    inotify_rm_watch(fd_, wd_);
    close(fd_);
    close(wd_);
}
