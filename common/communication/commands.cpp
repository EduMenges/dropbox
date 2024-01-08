#include "commands.hpp"
#include <unordered_map>

using namespace dropbox::command_internal;

std::optional<dropbox::Command> dropbox::CommandFromStr(std::string_view str) noexcept {
    static const std::unordered_map<std::string_view, Command> kCommandMap = {{kUpload, Command::kUpload},
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
