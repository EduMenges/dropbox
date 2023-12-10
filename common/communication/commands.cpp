#include "commands.hpp"
#include <unordered_map>

namespace dropbox {
static constexpr const char* kUpload     = "upload";
static constexpr const char* kDownload   = "download";
static constexpr const char* kDelete     = "delete";
static constexpr const char* kListServer = "list_server";
static constexpr const char* kListClient = "list_client";
static constexpr const char* kGetSyncDir = "get_sync_dir";
static constexpr const char* kExit       = "exit";
static constexpr const char* kError      = "error";
static constexpr const char* kSuccess    = "success";
static constexpr const char* kUsername   = "username";
}

std::optional<dropbox::Command> dropbox::CommandFromStr(const std::string& str) {
    static const std::unordered_map<std::string, Command> kCommandMap = {{kUpload, Command::kUpload},
                                                                         {kDownload, Command::kDownload},
                                                                         {kDelete, Command::kDelete},
                                                                         {kListServer, Command::kListServer},
                                                                         {kListClient, Command::kListClient},
                                                                         {kGetSyncDir, Command::kGetSyncDir},
                                                                         {kExit, Command::kExit},
                                                                         {kError, Command::kError},
                                                                         {kSuccess, Command::kSuccess},
                                                                         {kUsername, Command::kUsername}};

    try {
        return kCommandMap.at(str);
    } catch (std::exception& _e) {
        return std::nullopt;
    }
}

std::ostream& dropbox::operator<<(std::ostream& os, dropbox::Command command) {
    static const std::unordered_map<Command, const std::string> kCommandMap = {
        {Command::kUpload, kUpload},     {Command::kDownload, kDownload},       {Command::kDelete, kDelete},
        {Command::kExit, kExit},         {Command::kListClient, kListClient},  {Command::kListServer, kListServer},
        {Command::kUsername, kUsername}, {Command::kGetSyncDir, kGetSyncDir}, {Command::kSuccess, kSuccess},
        {Command::kError, kError}};

    os << kCommandMap.at(command);
    return os;
}
