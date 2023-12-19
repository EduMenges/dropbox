#pragma once

#include <exception>
#include <filesystem>

namespace dropbox {
class SocketCreation : public std::exception {
   public:
    SocketCreation() = default;

    [[nodiscard]] const char* what() const noexcept override { return "Failure when creating sockets"; }
};

class Binding : public std::exception {
   public:
    Binding() = default;

    [[nodiscard]] const char* what() const noexcept override { return "Failure when binding to socket"; }
};

class Connecting : public std::exception {
   public:
    Connecting() = default;

    [[nodiscard]] const char* what() const noexcept override { return "Failure when connecting to address"; }
};

class Listening : public std::exception {
   public:
    Listening() = default;

    [[nodiscard]] const char* what() const noexcept override { return "Failure when listening"; }
};

class Username : public std::exception {
   public:
    Username() = default;

    [[nodiscard]] const char* what() const noexcept override { return "Failure when exchanging username"; }
};

class FullList : public std::exception {
   public:
    FullList() = default;

    [[nodiscard]] const char* what() const noexcept override { return "Client list is full of devices"; }
};

class InotifyCreate : public std::system_error {
   public:
    InotifyCreate() : std::system_error(std::make_error_code(static_cast<std::errc>(errno))){};
};

class InotifyWatch : public std::filesystem::filesystem_error {
   public:
    InotifyWatch(const std::filesystem::path& path)
        : std::filesystem::filesystem_error("Could not watch directory", path,
                                            std::make_error_code(static_cast<std::errc>(errno))){};
};

}
