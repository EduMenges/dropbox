#pragma once

#include <sys/socket.h>

namespace dropbox {
constexpr int kDomain   = AF_INET;
constexpr int kFamily   = kDomain;
constexpr int kType     = SOCK_STREAM;
constexpr int kProtocol = 0;
}
