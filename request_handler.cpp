#include "request_handler.hpp"
#include "auth.hpp"
#include "tasks.hpp"

json process_request(
    const json& request,
    bool& logged_in,
    std::string& logged_username,
    bool& should_disconnect,
    std::mutex& users_mutex,
    std::mutex& tasks_mutex
) {
    if (!request.contains("action") || !request["action"].is_string()) {
        return {
            {"status", "error"},
            {"message", "missing or invalid action"}
        };
    }

    std::string action = request["action"];

    if (action == "register") {
        if (!request.contains("username") || !request["username"].is_string()) {
            return {
                {"status", "error"},
                {"message", "missing or invalid username"}
            };
        }

        if (!request.contains("password") || !request["password"].is_string()) {
            return {
                {"status", "error"},
                {"message", "missing or invalid password"}
            };
        }

        return register_user(request["username"], request["password"], users_mutex, tasks_mutex);
    }

    if (action == "login") {
        if (!request.contains("username") || !request["username"].is_string()) {
            return {
                {"status", "error"},
                {"message", "missing or invalid username"}
            };
        }

        if (!request.contains("password") || !request["password"].is_string()) {
            return {
                {"status", "error"},
                {"message", "missing or invalid password"}
            };
        }

        json result = login_user(request["username"], request["password"], users_mutex);
        if (result["status"] == "ok") {
            logged_in = true;
            logged_username = request["username"];
        }
        return result;
    }

    if (action == "logout") {
        if (!logged_in) {
            return {
                {"status", "error"},
                {"message", "you are not logged in"}
            };
        }

        logged_in = false;
        logged_username.clear();
        should_disconnect = true;

        return {
            {"status", "ok"},
            {"message", "logout successful"}
        };
    }

    if (!logged_in) {
        return {
            {"status", "error"},
            {"message", "please login first"}
        };
    }

    if (action == "list_available_tasks") {
        return list_available_tasks(logged_username, tasks_mutex);
    }

    if (action == "list_active_tasks") {
        return list_active_tasks(logged_username, tasks_mutex);
    }

    if (action == "add_task") {
        if (!request.contains("task") || !request["task"].is_string()) {
            return {
                {"status", "error"},
                {"message", "missing or invalid task"}
            };
        }

        return add_task_to_user(logged_username, request["task"], tasks_mutex);
    }

    if (action == "remove_task") {
        if (!request.contains("task") || !request["task"].is_string()) {
            return {
                {"status", "error"},
                {"message", "missing or invalid task"}
            };
        }

        return remove_task_from_user(logged_username, request["task"], tasks_mutex);
    }

    if (action == "execute_task") {
        if (!request.contains("task") || !request["task"].is_string()) {
            return {
                {"status", "error"},
                {"message", "missing or invalid task"}
            };
        }

        return execute_task_for_user(logged_username, request["task"], tasks_mutex);
    }

    return {
        {"status", "error"},
        {"message", "unknown action"}
    };
}