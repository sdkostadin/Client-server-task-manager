#pragma once

#include <string>
#include <mutex>

enum class LogLevel {
    INFO,
    WARN,
    ERROR
};

void init_logger(const std::string& filename);
void log_message(LogLevel level, const std::string& message);