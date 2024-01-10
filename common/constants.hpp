#pragma once

#include <sys/types.h>

namespace dropbox {

constexpr int     kInvalidSocket  = -1;  ///< Invalid socket.
constexpr int     kInvalidConnect = -1;  ///< Invalid connect.
constexpr ssize_t kInvalidWrite   = -1;  ///< Invalid write.
constexpr ssize_t kInvalidRead    = -1;  ///< Invalid read.

/// Max size of a single packet exchange.
static constexpr size_t kPacketSize = 64U * 1024U;
}
