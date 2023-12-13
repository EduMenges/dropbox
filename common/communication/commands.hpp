#pragma once

#include <optional>
#include <ostream>

namespace dropbox {

/// Possible user actions and/or states from their actions.
enum class Command: int8_t {
    kError = -1,         ///< An error occurred.
    kSuccess,        ///< Operation was a success.
    kUpload,        ///< Uploads a file from \c cwd to the root directory.
    kDelete,        ///< Deletes a file at the root directory.
    kUsername,      ///< Username receiver.
    kGetSyncDir,  ///< Downloads the \c sync_dir directory and starts syncing.
    kExit,          ///< Ends connection with server
    kListClient,   ///< Lists the files from the client
    kListServer,   ///< Lists the files from the server
    kDownload,      ///< Downloads a file to the \c cwd.
    kWriteDir,
    kDeleteDir
};

/// Constructs a command based on a str.
std::optional<Command> CommandFromStr(const std::string& str);

/// Used for printing to streams.
std::ostream& operator<<(std::ostream& os, Command command);

}
