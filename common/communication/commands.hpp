#pragma once

#include "fmt/format.h"
#include <optional>
#include <ostream>

namespace dropbox {

/// Possible user actions and/or states from their actions.
enum class Command : int8_t {
    kError = -1,  ///< An error occurred.
    kSuccess,     ///< Operation was a success.
    kGetSyncDir,  ///< Downloads the \c sync_dir directory and starts syncing.
    kUpload,      ///< Uploads a file from \c cwd to the root directory.
    kDownload,    ///< Downloads a file to the \c cwd.
    kDelete,      ///< Deletes a file at the root directory.
    kUsername,    ///< Username receiver.
    kListClient,  ///< Lists the files from the client.
    kListServer,  ///< Lists the files from the server.
    kExit,        ///< Ends connection with server.
};

/// Constructs a command based on a str.
std::optional<Command> CommandFromStr(std::string_view str) noexcept;

/// @internal Only for use with @ref StrFromCommand or @ref CommandFromStr
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

/**
 * Constructs a string from the @p command.
 * @param command Command to construct the string from.
 * @return The constructed string.
 */
constexpr std::string_view StrFromCommand(Command command) noexcept {
    std::string_view ans;

    switch (command) {
        case Command::kError:
            ans = command_internal::kError;
            break;
        case Command::kSuccess:
            ans = command_internal::kSuccess;
            break;
        case Command::kUpload:
            ans = command_internal::kUpload;
            break;
        case Command::kDelete:
            ans = command_internal::kDelete;
            break;
        case Command::kUsername:
            ans = command_internal::kUsername;
            break;
        case Command::kGetSyncDir:
            ans = command_internal::kGetSyncDir;
            break;
        case Command::kExit:
            ans = command_internal::kExit;
            break;
        case Command::kListClient:
            ans = command_internal::kListClient;
            break;
        case Command::kListServer:
            ans = command_internal::kListServer;
            break;
        case Command::kDownload:
            ans = command_internal::kDownload;
            break;
    }

    return ans;
}
}

template <>
struct fmt::formatter<dropbox::Command> : formatter<string_view> {
   public:
    constexpr auto format(dropbox::Command command, format_context& ctx) const {
        return formatter<string_view>::format(StrFromCommand(command), ctx);
    }
};
