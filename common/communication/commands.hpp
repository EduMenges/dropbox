#pragma once

#include <optional>
#include <ostream>

namespace dropbox {

/// Possible user actions
enum class Command {
    UPLOAD,        ///< Uploads a file from \c cwd to the root directory.
    DELETE,        ///< Deletes a file at the root directory.
    USERNAME,      ///< Username receiver.
    GET_SYNC_DIR,  ///< Downloads the \c sync_dir directory and starts syncing.
    EXIT,          ///< Ends connection with server
    LIST_CLIENT,   ///< Lists the files from the client
    LIST_SERVER,   ///< Lists the files from the server
    DOWNLOAD,      ///< Downloads a file to the \c cwd.
    ERROR,         ///< An error occurred.
    SUCCESS,        ///< Operation was a success.
    WRITE_DIR,
    DELETE_DIR
};

/// Constructs a command based on a str.
std::optional<Command> CommandFromStr(const std::string& str);

/// Used for printing to streams.
std::ostream& operator<<(std::ostream& os, Command command);

}
