#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <ctime>

class RateLimiter {
public:
    RateLimiter(int max_attempts = 5, int block_seconds = 30);

    bool can_attempt_login(const std::string& username, std::string& message);
    void record_failed_attempt(const std::string& username);
    void reset_attempts(const std::string& username);

private:
    struct AttemptInfo {
        int failed_count = 0;
        std::time_t blocked_until = 0;
    };

    std::unordered_map<std::string, AttemptInfo> attempts_;
    std::mutex mutex_;
    int max_attempts_;
    int block_seconds_;
};