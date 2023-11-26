#pragma once

#include <filesystem>

#include "tabulate/table.hpp"

namespace dropbox {
/**
 * @brief Constructs a table for displaying.
 * @pre \p path points to a directory.
 */
tabulate::Table ListDirectory(const std::filesystem::path& path);
}
