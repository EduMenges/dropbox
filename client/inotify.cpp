#include "inotify.hpp"

#include <unistd.h>

#include <iostream>

#include "utils.hpp"

dropbox::Inotify::Inotify(std::filesystem::path &&watch_path)
    : watch_path_(std::move(watch_path)), fd_(inotify_init1(IN_NONBLOCK)), wd_(-1) {
    if (fd_ == -1) {
        throw InotifyCreate();
    }
}

void dropbox::Inotify::Start() {
    wd_ = inotify_add_watch(fd_, watch_path_.c_str(), IN_DELETE | IN_MOVE | IN_CLOSE_WRITE | IN_MODIFY);

    if (wd_ == -1) {
        throw InotifyWatch(watch_path_);
    }

    watching_ = true;
    pause_ = false;
}

dropbox::Inotify::~Inotify() {
    inotify_rm_watch(fd_, wd_);
    close(wd_);
    close(fd_);
}

void dropbox::Inotify::MainLoop(const std::stop_token &stop_token) {
    static thread_local std::array<uint8_t, kBufferLength> buffer;

    while (watching_ && !stop_token.stop_requested()) {
        const ssize_t kLength = read(fd_, buffer.data(), buffer.size());

        if (kLength == kInvalidRead || pause_) {
            if (errno != EWOULDBLOCK) {
                perror(__func__);
            }
            continue;
        }

        size_t i = 0;

        collection_mutex_.lock();
        while (static_cast<ssize_t>(i) < kLength) {
            auto &event = *reinterpret_cast<struct inotify_event *>(&buffer[i]);

            if (event.len != 0U) {
                if ((event.mask & (IN_CLOSE_WRITE | IN_MOVED_TO)) != 0U) {
                    if ((event.mask & IN_ISDIR) == 0U) {
                        vector_.emplace_back(Command::kUpload, event.name);
                    }
                } else if ((event.mask & (IN_DELETE | IN_MOVED_FROM)) != 0U) {
                    if ((event.mask & IN_ISDIR) == 0U) {
                        vector_.emplace_back(Command::kDelete, event.name);
                    }
                }
            }
            i += kEventSize + event.len;
        }
        collection_mutex_.unlock();

        cv_.notify_one();
    }
    cv_.notify_one();
}
