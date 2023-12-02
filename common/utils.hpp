#pragma once

#include <filesystem>

#include "constants.hpp"

namespace dropbox {

/// Sync dir prefix to concatenate the paths with.
static constexpr const char* kSyncDirPrefix = "./sync_dir_";

/// Path to the sync dir based on an username.
inline std::filesystem::path SyncDirWithPrefix(const std::string& username) {
    return std::filesystem::path(kSyncDirPrefix).concat(username);
}
}
