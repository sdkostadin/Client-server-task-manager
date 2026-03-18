#pragma once
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool is_valid_password(const std::string& password, std::string& error_message);
std::string hash_password_sha256(const std::string& password);

json ensure_admin_exists(std::mutex& users_mutex);
bool is_admin_user(const std::string& username, std::mutex& users_mutex);

json register_user(const std::string& username, const std::string& password,
                   std::mutex& users_mutex, std::mutex& tasks_mutex);

json login_user(const std::string& username, const std::string& password,
                std::mutex& users_mutex);

json list_users(std::mutex& users_mutex);