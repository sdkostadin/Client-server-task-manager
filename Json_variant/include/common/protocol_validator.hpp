#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool validate_action_field(const json& request, json& error_response);
bool validate_string_field(const json& request, const std::string& field_name, json& error_response);