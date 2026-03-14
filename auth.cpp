#include "auth.hpp"
#include "json_utils.hpp"
#include <cctype>

json ensure_tasks_entry_exists(const std::string& username, std::mutex& tasks_mutex) {
    std::lock_guard<std::mutex> lock(tasks_mutex);

    json data = load_json_or_default("tasks.json", {
        {"users_tasks", json::array()}
    });

    if (!data.contains("users_tasks") || !data["users_tasks"].is_array()) {
        data["users_tasks"] = json::array();
    }

    for (const auto& entry : data["users_tasks"]) {
        if (entry.contains("username") && entry["username"] == username) {
            return {
                {"status", "ok"},
                {"message", "tasks entry already exists"}
            };
        }
    }

    data["users_tasks"].push_back({
        {"username", username},
        {"available_tasks", json::array({
            "read_reports",
            "create_invoice",
            "approve_request"
        })},
        {"active_tasks", json::array()}
    });

    if (!save_json_to_file("tasks.json", data)) {
        return {
            {"status", "error"},
            {"message", "failed to initialize tasks.json"}
        };
    }

    return {
        {"status", "ok"},
        {"message", "tasks entry created"}
    };
}

bool is_valid_password(const std::string& password, std::string& error_message) {
    if (password.length() < 8) {
        error_message = "password must be at least 8 characters long";
        return false;
    }

    bool has_upper = false;
    bool has_lower = false;

    for (char ch : password) {
        if (std::isupper(static_cast<unsigned char>(ch))) {
            has_upper = true;
        }
        if (std::islower(static_cast<unsigned char>(ch))) {
            has_lower = true;
        }
    }

    if (!has_upper) {
        error_message = "password must contain at least one uppercase letter";
        return false;
    }

    if (!has_lower) {
        error_message = "password must contain at least one lowercase letter";
        return false;
    }

    return true;
}

json register_user(const std::string& username, const std::string& password, std::mutex& users_mutex, std::mutex& tasks_mutex) {
    std::string password_error;
    if (!is_valid_password(password, password_error)) {
        return {
            {"status", "error"},
            {"message", password_error}
        };
    }

    {
        std::lock_guard<std::mutex> lock(users_mutex);

        json data = load_json_or_default("users.json", {
            {"users", json::array()}
        });

        if (!data.contains("users") || !data["users"].is_array()) {
            data["users"] = json::array();
        }

        for (const auto& user : data["users"]) {
            if (user.contains("username") && user["username"] == username) {
                return {
                    {"status", "error"},
                    {"message", "username already exists"}
                };
            }
        }

        data["users"].push_back({
            {"username", username},
            {"password", password}
        });

        if (!save_json_to_file("users.json", data)) {
            return {
                {"status", "error"},
                {"message", "failed to write users.json"}
            };
        }
    }

    json task_init_result = ensure_tasks_entry_exists(username, tasks_mutex);
    if (task_init_result["status"] == "error") {
        return task_init_result;
    }

    return {
        {"status", "ok"},
        {"message", "user registered"}
    };
}

json login_user(const std::string& username, const std::string& password, std::mutex& users_mutex) {
    std::lock_guard<std::mutex> lock(users_mutex);

    json data = load_json_or_default("users.json", {
        {"users", json::array()}
    });

    if (!data.contains("users") || !data["users"].is_array()) {
        return {
            {"status", "error"},
            {"message", "invalid users.json format"}
        };
    }

    for (const auto& user : data["users"]) {
        if (user.contains("username") &&
            user.contains("password") &&
            user["username"] == username &&
            user["password"] == password) {
            return {
                {"status", "ok"},
                {"message", "login successful"}
            };
        }
    }

    return {
        {"status", "error"},
        {"message", "invalid username or password"}
    };
}