#include "tasks/tasks.hpp"
#include "common/json_utils.hpp"
#include "tasks/task_executor.hpp"

#include <algorithm>

json list_available_tasks(const std::string& username, std::mutex& tasks_mutex) {
    std::lock_guard<std::mutex> lock(tasks_mutex);

    json data = load_json_or_default("data/tasks.json", {
        {"users_tasks", json::array()}
    });

    if (!data.contains("users_tasks") || !data["users_tasks"].is_array()) {
        return {
            {"status", "error"},
            {"message", "invalid tasks.json format"}
        };
    }

    for (const auto& entry : data["users_tasks"]) {
        if (entry.value("username", "") == username) {
            return {
                {"status", "ok"},
                {"available_tasks", entry.value("available_tasks", json::array())}
            };
        }
    }

    return {
        {"status", "error"},
        {"message", "user tasks not found"}
    };
}

json list_active_tasks(const std::string& username, std::mutex& tasks_mutex) {
    std::lock_guard<std::mutex> lock(tasks_mutex);

    json data = load_json_or_default("data/tasks.json", {
        {"users_tasks", json::array()}
    });

    if (!data.contains("users_tasks") || !data["users_tasks"].is_array()) {
        return {
            {"status", "error"},
            {"message", "invalid tasks.json format"}
        };
    }

    for (const auto& entry : data["users_tasks"]) {
        if (entry.value("username", "") == username) {
            return {
                {"status", "ok"},
                {"active_tasks", entry.value("active_tasks", json::array())}
            };
        }
    }

    return {
        {"status", "error"},
        {"message", "user tasks not found"}
    };
}

json add_task_to_user(const std::string& username, const std::string& task_name, std::mutex& tasks_mutex) {
    std::lock_guard<std::mutex> lock(tasks_mutex);

    json data = load_json_or_default("data/tasks.json", {
        {"users_tasks", json::array()}
    });

    if (!data.contains("users_tasks") || !data["users_tasks"].is_array()) {
        return {
            {"status", "error"},
            {"message", "invalid tasks.json format"}
        };
    }

    for (auto& entry : data["users_tasks"]) {
        if (entry.value("username", "") == username) {
            auto& available = entry["available_tasks"];
            auto& active = entry["active_tasks"];

            bool allowed = false;
            for (const auto& t : available) {
                if (t == task_name) {
                    allowed = true;
                    break;
                }
            }

            if (!allowed) {
                return {
                    {"status", "error"},
                    {"message", "task is not allowed for this user"}
                };
            }

            for (const auto& t : active) {
                if (t == task_name) {
                    return {
                        {"status", "error"},
                        {"message", "task already added"}
                    };
                }
            }

            active.push_back(task_name);

            if (!save_json_to_file_atomic("data/tasks.json", data)) {
                return {
                    {"status", "error"},
                    {"message", "failed to write tasks.json"}
                };
            }

            return {
                {"status", "ok"},
                {"message", "task added"}
            };
        }
    }

    return {
        {"status", "error"},
        {"message", "user tasks not found"}
    };
}

json remove_task_from_user(const std::string& username, const std::string& task_name, std::mutex& tasks_mutex) {
    std::lock_guard<std::mutex> lock(tasks_mutex);

    json data = load_json_or_default("data/tasks.json", {
        {"users_tasks", json::array()}
    });

    if (!data.contains("users_tasks") || !data["users_tasks"].is_array()) {
        return {
            {"status", "error"},
            {"message", "invalid tasks.json format"}
        };
    }

    for (auto& entry : data["users_tasks"]) {
        if (entry.value("username", "") == username) {
            auto& active = entry["active_tasks"];
            auto it = std::find(active.begin(), active.end(), task_name);

            if (it == active.end()) {
                return {
                    {"status", "error"},
                    {"message", "task not found in active tasks"}
                };
            }

            active.erase(it);

            if (!save_json_to_file_atomic("data/tasks.json", data)) {
                return {
                    {"status", "error"},
                    {"message", "failed to write tasks.json"}
                };
            }

            return {
                {"status", "ok"},
                {"message", "task removed"}
            };
        }
    }

    return {
        {"status", "error"},
        {"message", "user tasks not found"}
    };
}

json execute_task_for_user(const std::string& username, const std::string& task_name, std::mutex& tasks_mutex) {
    std::lock_guard<std::mutex> lock(tasks_mutex);

    json data = load_json_or_default("data/tasks.json", {
        {"users_tasks", json::array()}
    });

    if (!data.contains("users_tasks") || !data["users_tasks"].is_array()) {
        return {
            {"status", "error"},
            {"message", "invalid tasks.json format"}
        };
    }

    for (const auto& entry : data["users_tasks"]) {
        if (entry.value("username", "") == username) {
            const auto& active = entry["active_tasks"];

            for (const auto& t : active) {
                if (t == task_name) {
                    return run_task_handler(username, task_name);
                }
            }

            return {
                {"status", "error"},
                {"message", "task is not active for this user"}
            };
        }
    }

    return {
        {"status", "error"},
        {"message", "user tasks not found"}
    };
}

json assign_task_to_user(const std::string& target_username, const std::string& task_name, std::mutex& tasks_mutex) {
    std::lock_guard<std::mutex> lock(tasks_mutex);

    json data = load_json_or_default("data/tasks.json", {
        {"users_tasks", json::array()}
    });

    if (!data.contains("users_tasks") || !data["users_tasks"].is_array()) {
        return {
            {"status", "error"},
            {"message", "invalid tasks.json format"}
        };
    }

    for (auto& entry : data["users_tasks"]) {
        if (entry.value("username", "") == target_username) {
            auto& available = entry["available_tasks"];

            for (const auto& t : available) {
                if (t == task_name) {
                    return {
                        {"status", "error"},
                        {"message", "task already assigned to user"}
                    };
                }
            }

            available.push_back(task_name);

            if (!save_json_to_file_atomic("data/tasks.json", data)) {
                return {
                    {"status", "error"},
                    {"message", "failed to write tasks.json"}
                };
            }

            return {
                {"status", "ok"},
                {"message", "task assigned to user"}
            };
        }
    }

    return {
        {"status", "error"},
        {"message", "target user not found in tasks.json"}
    };
}

int count_total_active_tasks(std::mutex& tasks_mutex) {
    std::lock_guard<std::mutex> lock(tasks_mutex);

    json data = load_json_or_default("data/tasks.json", {
        {"users_tasks", json::array()}
    });

    if (!data.contains("users_tasks") || !data["users_tasks"].is_array()) {
        return 0;
    }

    int total = 0;
    for (const auto& entry : data["users_tasks"]) {
        if (entry.contains("active_tasks") && entry["active_tasks"].is_array()) {
            total += static_cast<int>(entry["active_tasks"].size());
        }
    }

    return total;
}