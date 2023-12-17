#pragma once

#include <filesystem>

#include "constants.hpp"

namespace dropbox {

/// Sync dir prefix to concatenate the paths with.
static constexpr const char* kSyncDirPrefix = "sync_dir_";

/// Path to the sync dir based on an username.
inline std::filesystem::path SyncDirWithPrefix(const std::string& username) {
    return std::filesystem::path(kSyncDirPrefix).concat(username);
}

template <typename T>
ssize_t SSizeOf()
{
    return static_cast<ssize_t>(sizeof(T));
}

template <typename T>
ssize_t SSizeOf(const T& _instance)
{
    return SSizeOf<T>();
}

inline void RemoveAllFromDirectory(const std::filesystem::path& path) {
    for (const auto& entry: std::filesystem::directory_iterator(path)) {
        std::filesystem::remove_all(entry.path());
    }
}
}
