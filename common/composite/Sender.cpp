#include "Sender.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/string.hpp"

bool dropbox::composite::Sender::Upload(const std::filesystem::path& path) {
    exchange_.SendCommand(Command::kUpload);

    if (!exchange_.SetPath(path).SendPath()) {
        return false;
    }

    if (!exchange_.Send()) {
        return false;
    }

    exchange_.Flush();
    return true;
}

bool dropbox::composite::Sender::Delete(const std::filesystem::path& path) {
    exchange_.SendCommand(Command::kDelete);

    if (!exchange_.SetPath(path).SendPath()) {
        return false;
    }

    exchange_.Flush();
    return true;
}

bool dropbox::composite::Sender::Ip(std::string ip) {
    exchange_.SendCommand(Command::kSuccess);

    cereal::PortableBinaryOutputArchive archive(stream_);
    archive(ip);
    stream_.flush();

    return true;
}
