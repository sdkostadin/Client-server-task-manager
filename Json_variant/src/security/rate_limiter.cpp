#include "security/rate_limiter.hpp"
#include <sstream>

RateLimiter::RateLimiter(int max_attempts, int block_seconds)
    : max_attempts_(max_attempts), block_seconds_(block_seconds) {
}

bool RateLimiter::can_attempt_login(const std::string& username, std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::time_t now = std::time(nullptr);
    auto& info = attempts_[username];

    if (info.blocked_until > now) {
        int seconds_left = static_cast<int>(info.blocked_until - now);

        std::ostringstream oss;
        oss << "too many failed attempts; try again in " << seconds_left << " seconds";
        message = oss.str();

        return false;
    }

    if (info.blocked_until != 0 && info.blocked_until <= now) {
        info.blocked_until = 0;
        info.failed_count = 0;
    }

    return true;
}

void RateLimiter::record_failed_attempt(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& info = attempts_[username];
    info.failed_count++;

    if (info.failed_count >= max_attempts_) {
        info.blocked_until = std::time(nullptr) + block_seconds_;
        info.failed_count = 0;
    }
}

void RateLimiter::reset_attempts(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    attempts_.erase(username);
}