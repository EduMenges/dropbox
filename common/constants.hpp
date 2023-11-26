#pragma once

#include <sys/types.h>

#include <filesystem>

namespace dropbox {
static constexpr const char* kSyncDirPrefix = "sync_dir_";

inline std::string getSyncDir(const char* username) { return std::string(kSyncDirPrefix) + username; }

constexpr int     kInvalidSocket = -1;
constexpr ssize_t kInvalidWrite  = -1;
constexpr ssize_t kInvalidRead   = -1;
}
