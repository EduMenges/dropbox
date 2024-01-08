#pragma once

#include "fmt/format.h"
#include <optional>
#include <ostream>

namespace dropbox {

/// Possible user actions and/or states from their actions.
enum class Command : int8_t {
    kError = -1,  ///< An error occurred.
    kSuccess,     ///< Operation was a success.
    kUpload,      ///< Uploads a file from \c cwd to the root directory.
    kDelete,      ///< Deletes a file at the root directory.
    kUsername,    ///< Username receiver.
    kGetSyncDir,  ///< Downloads the \c sync_dir directory and starts syncing.
    kExit,        ///< Ends connection with server
    kListClient,  ///< Lists the files from the client
    kListServer,  ///< Lists the files from the server
    kDownload,    ///< Downloads a file to the \c cwd.
};

/// Constructs a command based on a str.
std::optional<Command> CommandFromStr(std::string_view str) noexcept;

namespace command_internal {
constexpr std::string_view kUpload     = "upload";
constexpr std::string_view kDownload   = "download";
constexpr std::string_view kDelete     = "delete";
constexpr std::string_view kListServer = "list_server";
constexpr std::string_view kListClient = "list_client";
constexpr std::string_view kGetSyncDir = "get_sync_dir";
constexpr std::string_view kExit       = "exit";
constexpr std::string_view kError      = "error";
constexpr std::string_view kSuccess    = "success";
constexpr std::string_view kUsername   = "username";
}

constexpr std::string_view StrFromCommand(Command command) noexcept {
    switch (command) {
        case Command::kError:
            return command_internal::kError;
        case Command::kSuccess:
            return command_internal::kSuccess;
        case Command::kUpload:
            return command_internal::kUpload;
        case Command::kDelete:
            return command_internal::kDelete;
        case Command::kUsername:
            return command_internal::kUsername;
        case Command::kGetSyncDir:
            return command_internal::kGetSyncDir;
        case Command::kExit:
            return command_internal::kExit;
        case Command::kListClient:
            return command_internal::kListClient;
        case Command::kListServer:
            return command_internal::kListServer;
        case Command::kDownload:
            return command_internal::kDownload;
    }
}
}

template <>
struct fmt::formatter<dropbox::Command> : formatter<string_view> {
   public:
    constexpr auto format(dropbox::Command command, format_context& ctx) const {
        return formatter<string_view>::format(StrFromCommand(command), ctx);
    }
};
