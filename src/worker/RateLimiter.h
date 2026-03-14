#pragma once
#include <mutex>
#include <chrono>
#include <thread>

// Rate limiter: allows N operations per minute
// mode: 0 = unlimited (fire at will), N>0 = N per minute
class RateLimiter {
public:
    void setRate(int per_minute) {
        std::lock_guard lk(mtx_);
        per_minute_ = per_minute;
        if (per_minute > 0) {
            interval_ = std::chrono::milliseconds(60000 / per_minute);
        }
    }

    int rate() const { return per_minute_; }

    // Call before each operation. Blocks if needed.
    void acquire() {
        if (per_minute_ <= 0) return;  // unlimited
        std::lock_guard lk(mtx_);
        auto now = std::chrono::steady_clock::now();
        if (now < next_allowed_) {
            std::this_thread::sleep_until(next_allowed_);
            now = std::chrono::steady_clock::now();
        }
        next_allowed_ = now + interval_;
    }

private:
    int per_minute_ = 0;
    std::chrono::milliseconds interval_{0};
    std::chrono::steady_clock::time_point next_allowed_;
    std::mutex mtx_;
};
