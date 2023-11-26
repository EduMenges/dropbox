#pragma once

#include <cstddef>

namespace dropbox {

template <typename T>
size_t TotalSize(const T& collection) {
    return sizeof(T) * collection.size();
}
}
