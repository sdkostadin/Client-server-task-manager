#pragma once
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json process_request(
    const json& request,
    bool& logged_in,
    std::string& logged_username,
    std::string& logged_role,
    bool& should_disconnect,
    std::mutex& users_mutex,
    std::mutex& tasks_mutex
);