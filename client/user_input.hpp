#pragma once

#include <thread>
#include <queue>

namespace dropbox{
class UserInput{
   public:
    UserInput();

    void Start();
    void Stop();
    std::string GetQueue();

   private:
    std::thread input_thread_;
    bool reading_;

    std::queue<std::string> input_queue_;
};
}
