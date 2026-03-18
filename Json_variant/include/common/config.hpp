#pragma once

#include <string>

struct ServerConfig {
    int port = 8080;
    int backlog = 16;
    std::string log_file = "logs/server.log";
    int login_max_attempts = 5;
    int login_block_seconds = 30;
    int accept_poll_timeout_ms = 1000;
    int shutdown_drain_seconds = 5;
};

ServerConfig load_config(const std::string& filename);