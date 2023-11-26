#pragma once

#include <sys/types.h>

namespace dropbox {
static constexpr const char* kSyncDirPrefix = "./sync_dir_";
static constexpr const char* kSyncDirPath = "./server/dir/";

constexpr int     kInvalidSocket = -1;
constexpr ssize_t kInvalidWrite  = -1;
constexpr ssize_t kInvalidRead   = -1;
}
