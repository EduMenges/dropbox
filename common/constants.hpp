#pragma once

#include <sys/types.h>

namespace dropbox {

static constexpr const char* kSyncDirPrefix = "./sync_dir_";
static constexpr const char* kSyncDirPath = "./server/dir/";

constexpr int     kInvalidSocket = -1;
constexpr ssize_t kInvalidWrite  = -1;
constexpr ssize_t kInvalidRead   = -1;

/// Max size of a single packet exchange.
static constexpr size_t kPacketSize = 64U * 1024U;
}

#include <semaphore>
static sem_t sem_server_, sem_client_;