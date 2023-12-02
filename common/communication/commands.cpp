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
    static const std::unordered_map<std::string, Command> kCommandMap = {{kUpload, Command::UPLOAD},
                                                                         {kDownload, Command::DOWNLOAD},
                                                                         {kDelete, Command::DELETE},
                                                                         {kListServer, Command::LIST_SERVER},
                                                                         {kListClient, Command::LIST_CLIENT},
                                                                         {kGetSyncDir, Command::GET_SYNC_DIR},
                                                                         {kExit, Command::EXIT},
                                                                         {kError, Command::ERROR},
                                                                         {kSuccess, Command::SUCCESS},
                                                                         {kUsername, Command::USERNAME}};

    try {
        return kCommandMap.at(str);
    } catch (std::exception& _e) {
        return std::nullopt;
    }
}

std::ostream& dropbox::operator<<(std::ostream& os, dropbox::Command command) {
    static const std::unordered_map<Command, const std::string> kCommandMap = {
        {Command::UPLOAD, kUpload},     {Command::DOWNLOAD, kDownload},       {Command::DELETE, kDelete},
        {Command::EXIT, kExit},         {Command::LIST_CLIENT, kListClient},  {Command::LIST_SERVER, kListServer},
        {Command::USERNAME, kUsername}, {Command::GET_SYNC_DIR, kGetSyncDir}, {Command::SUCCESS, kSuccess},
        {Command::ERROR, kError}};

    os << kCommandMap.at(command);
    return os;
}
