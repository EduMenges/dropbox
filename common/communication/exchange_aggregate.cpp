#include "exchange_aggregate.hpp"

bool dropbox::ExchangeAggregate::Upload(std::filesystem::path&& path) {
    if (std::filesystem::is_directory(path)) {
        directory_exchange_.SetPath(std::move(path));
        return directory_exchange_.Send();
    } else {
        file_exchange_.SetPath(std::move(path));
        return file_exchange_.Send();
    }
}
