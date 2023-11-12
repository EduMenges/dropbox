#pragma once

#include <exception>

namespace dropbox
{
class SocketCreation : public std::exception {
 public:
  SocketCreation() = default;

  [[nodiscard]] const char* what() const noexcept override {
    return "Failure when creating sockets";
  }
};

class Binding: public std::exception {
 public:
  Binding() = default;

  [[nodiscard]] const char* what() const noexcept override {
    return "Failure when binding to socket";
  }
};
}
