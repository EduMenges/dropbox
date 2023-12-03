#include "connections.hpp"

#include <cstdio>
#include <fcntl.h>

bool dropbox::SetNonblocking(int socket) {
    const int kFlags = fcntl(socket, F_GETFL, 0);
    if (kFlags == -1) {
        perror(__func__);
        return false;
    }

    if (fcntl(socket, F_SETFL, kFlags | O_NONBLOCK) == -1) {
        perror(__func__);
        return false;
    }

    return true;
}
