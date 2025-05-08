#include "../include/execution/algorithms/timer.hpp"

namespace trading {
namespace execution {
namespace algorithms {

Timer::Timer(std::function<void(void)> callback, std::chrono::milliseconds interval)
    : callback_(callback), interval_(interval), running_(false) {}

Timer::~Timer() {
    stop();
}

void Timer::start() {
    running_ = true;
    thread_ = std::thread([this]() {
        while (running_) {
            std::this_thread::sleep_for(interval_);
            if (running_) {
                callback_();
            }
        }
    });
}

void Timer::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

} // namespace algorithms
} // namespace execution
} // namespace trading