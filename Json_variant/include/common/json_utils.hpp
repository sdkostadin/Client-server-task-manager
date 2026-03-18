#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json load_json_or_default(const std::string& filename, const json& default_value);
bool save_json_to_file_atomic(const std::string& filename, const json& data);