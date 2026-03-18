#pragma once

#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json list_available_tasks(const std::string& username, std::mutex& tasks_mutex);
json list_active_tasks(const std::string& username, std::mutex& tasks_mutex);
json add_task_to_user(const std::string& username, const std::string& task_name, std::mutex& tasks_mutex);
json remove_task_from_user(const std::string& username, const std::string& task_name, std::mutex& tasks_mutex);
json execute_task_for_user(const std::string& username, const std::string& task_name, std::mutex& tasks_mutex);
json assign_task_to_user(const std::string& target_username, const std::string& task_name, std::mutex& tasks_mutex);

int count_total_active_tasks(std::mutex& tasks_mutex);