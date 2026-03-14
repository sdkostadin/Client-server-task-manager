#pragma once

#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool is_valid_password(const std::string& password, std::string& error_message);
json register_user(const std::string& username, const std::string& password, std::mutex& users_mutex, std::mutex& tasks_mutex);
json login_user(const std::string& username, const std::string& password, std::mutex& users_mutex);