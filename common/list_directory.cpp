#include "list_directory.hpp"

#include <sys/stat.h>

#include "date/date.h"

namespace fs = std::filesystem;

namespace dropbox {

/**
 * Converts a C \c timespec into a C++ \c time_point.
 * @param ts \c timespec to be converted.
 * @return Equivalent \c time_point.
 */
static constexpr auto TimeSpecToTimePoint(timespec ts) {
    return std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(ts.tv_sec) +
                                                              std::chrono::nanoseconds(ts.tv_nsec));
}

/**
 * Converts a \c time_point into a readable format.
 * @param tp \c time_point to be converted.
 * @return Readable \p tp.
 */
static auto ReadableTimePoint(std::chrono::time_point<std::chrono::system_clock> tp) {
    return date::format("%Y-%m-%d %H:%M:%S", tp);
}

/**
 * Builds a row based on a C++ \c directory_entry.
 * @param entry Directory entry to build the row from.
 * @return Row with file name and MAC times.
 */
static tabulate::Table::Row_t RowFromEntry(const fs::directory_entry& entry) {
    struct stat file_info {};
    stat(entry.path().c_str(), &file_info);

    const auto kLastWriteTime  = std::chrono::file_clock::to_sys(fs::last_write_time(entry));
    const auto kLastAccessTime = TimeSpecToTimePoint(file_info.st_atim);
    const auto kCreationTime   = TimeSpecToTimePoint(file_info.st_ctim);

    return tabulate::Table::Row_t({entry.path().filename(), ReadableTimePoint(kLastWriteTime),
                                   ReadableTimePoint(kLastAccessTime), ReadableTimePoint(kCreationTime)});
}

}

tabulate::Table dropbox::ListDirectory(const std::filesystem::path& path) {
    tabulate::Table ret;

    ret.add_row({"File name", "Modification", "Access", "Creation"});

    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            ret.add_row(RowFromEntry(entry));
        }
    }

    return ret;
}
