#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json run_task_handler(const std::string& username, const std::string& task_name);