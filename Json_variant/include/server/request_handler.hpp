#pragma once

#include <string>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

#include "security/rate_limiter.hpp"
#include "session/session_registry.hpp"
#include "common/config.hpp"

using json = nlohmann::json;

json process_request(
    const json& request,
    bool& logged_in,
    std::string& logged_username,
    std::string& logged_role,
    bool& should_disconnect,
    std::atomic<bool>& stop_accepting_new_clients,
    std::mutex& users_mutex,
    std::mutex& tasks_mutex,
    RateLimiter& rate_limiter,
    SessionRegistry& session_registry,
    int session_id,
    const ServerConfig& config
);