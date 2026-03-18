#include "common/config.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ServerConfig load_config(const std::string& filename) {
    ServerConfig config;

    std::ifstream in(filename);
    if (!in.is_open()) {
        return config;
    }

    try {
        json data;
        in >> data;

        if (data.contains("port") && data["port"].is_number_integer()) {
            config.port = data["port"];
        }

        if (data.contains("backlog") && data["backlog"].is_number_integer()) {
            config.backlog = data["backlog"];
        }

        if (data.contains("log_file") && data["log_file"].is_string()) {
            config.log_file = data["log_file"];
        }

        if (data.contains("login_max_attempts") && data["login_max_attempts"].is_number_integer()) {
            config.login_max_attempts = data["login_max_attempts"];
        }

        if (data.contains("login_block_seconds") && data["login_block_seconds"].is_number_integer()) {
            config.login_block_seconds = data["login_block_seconds"];
        }

        if (data.contains("accept_poll_timeout_ms") && data["accept_poll_timeout_ms"].is_number_integer()) {
            config.accept_poll_timeout_ms = data["accept_poll_timeout_ms"];
        }

        if (data.contains("shutdown_drain_seconds") && data["shutdown_drain_seconds"].is_number_integer()) {
            config.shutdown_drain_seconds = data["shutdown_drain_seconds"];
        }
    } catch (...) {
        return config;
    }

    return config;
}