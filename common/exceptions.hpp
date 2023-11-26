#pragma once

#include <exception>

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

}
