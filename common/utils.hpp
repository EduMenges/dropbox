#pragma once

#include <cstddef>
#include <filesystem>

#include "constants.hpp"

namespace dropbox {

template <typename T>
size_t TotalSize(const T& collection) {
    return sizeof(T) * collection.size();
}

/// Path to the sync dir based on an username.
inline std::filesystem::path SyncDirWithPrefix(const std::string& username) {
    return std::filesystem::path(kSyncDirPrefix).concat(username);
}
}
