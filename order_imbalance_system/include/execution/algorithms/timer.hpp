#pragma once

#include <chrono>
#include <functional>
#include <thread>
#include <atomic>

namespace trading {
namespace execution {
namespace algorithms {

class Timer {
public:
    Timer(std::function<void(void)> callback, std::chrono::milliseconds interval);
    ~Timer();
    
    void start();
    void stop();
    
private:
    std::function<void(void)> callback_;
    std::chrono::milliseconds interval_;
    std::thread thread_;
    std::atomic<bool> running_;
};

} // namespace algorithms
} // namespace execution
} // namespace trading