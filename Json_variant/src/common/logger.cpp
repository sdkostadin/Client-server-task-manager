#include "common/logger.hpp"
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
    std::string g_log_filename = "logs/server.log";
    std::mutex g_log_mutex;

    std::string now_as_string() {
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
}

void init_logger(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    g_log_filename = filename;
}

void log_message(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_log_mutex);

    std::ofstream out(g_log_filename, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "Failed to open log file: " << g_log_filename << "\n";
        return;
    }

    out << "[" << now_as_string() << "] "
        << level_to_string(level) << " "
        << message << "\n";
}