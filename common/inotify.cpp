#include "inotify.hpp"

#include <unistd.h>

#include <iostream>

#include "utils.hpp"

dropbox::Inotify::Inotify(std::filesystem::path &&watch_path)
    : watch_path_(std::move(watch_path)), watching_(false), pause_(false), fd_(inotify_init()), wd_(-1) {
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
        mutex_.lock();
        while (i < length_) {
            auto &event = *reinterpret_cast<struct inotify_event *>(&buffer[i]);

            if (event.len != 0U) {
                if ((event.mask & (IN_CLOSE_WRITE | IN_MOVED_TO)) != 0U) {
                    if ((event.mask & IN_ISDIR) != 0U) {
                        std::cout << "The directory " << event.name << " was created/modified\n";
                    } else {
                        std::cout << "The file " << event.name << " was created/modified\n";
                        queue_.push({Command::kUpload, event.name});
                    }
                } else if ((event.mask & (IN_DELETE | IN_MOVED_FROM)) != 0U) {
                    if ((event.mask & IN_ISDIR) != 0U) {
                        std::cout << "The directory " << event.name << " was deleted\n";
                    } else {
                        std::cout << "The file " << event.name << " was deleted\n";
                        queue_.push({Command::kDelete, event.name});
                    }
                }
            }
            i += kEventSize + event.len;
        }
        mutex_.unlock();
        cv_.notify_one();
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
