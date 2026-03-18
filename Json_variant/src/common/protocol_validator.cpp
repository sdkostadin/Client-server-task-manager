#include "common/protocol_validator.hpp"

bool validate_action_field(const json& request, json& error_response) {
    if (!request.contains("action") || !request["action"].is_string()) {
        error_response = {
            {"status", "error"},
            {"message", "missing or invalid action"}
        };
        return false;
    }

    return true;
}

bool validate_string_field(const json& request, const std::string& field_name, json& error_response) {
    if (!request.contains(field_name) || !request[field_name].is_string()) {
        error_response = {
            {"status", "error"},
            {"message", "missing or invalid " + field_name}
        };
        return false;
    }

    return true;
}