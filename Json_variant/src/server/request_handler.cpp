#include "server/request_handler.hpp"
#include "auth/auth.hpp"
#include "tasks/tasks.hpp"
#include "common/logger.hpp"
#include "common/protocol_validator.hpp"

json process_request(
    const json& request,
    bool& logged_in,
    std::string& logged_username,
    std::string& logged_role,
    bool& should_disconnect,
    std::atomic<bool>& stop_accepting_new_clients,
    std::mutex& users_mutex,
    std::mutex& tasks_mutex,
    RateLimiter& rate_limiter,
    SessionRegistry& session_registry,
    int session_id,
    const ServerConfig& config
) {
    json validation_error;
    if (!validate_action_field(request, validation_error)) {
        return validation_error;
    }

    std::string action = request["action"];

    if (action == "register") {
        if (!validate_string_field(request, "username", validation_error)) {
            return validation_error;
        }

        if (!validate_string_field(request, "password", validation_error)) {
            return validation_error;
        }

        std::string username = request["username"];

        json result = register_user(
            username,
            request["password"],
            users_mutex,
            tasks_mutex
        );

        if (result["status"] == "ok") {
            log_message(LogLevel::INFO, "register success user=" + username);
        } else {
            log_message(LogLevel::WARN, "register failed user=" + username + " reason=" + result.value("message", "unknown"));
        }

        return result;
    }

    if (action == "login") {
        if (logged_in) {
            return {
                {"status", "error"},
                {"message", "already logged in"}
            };
        }

        if (!validate_string_field(request, "username", validation_error)) {
            return validation_error;
        }

        if (!validate_string_field(request, "password", validation_error)) {
            return validation_error;
        }

        std::string username = request["username"];

        json result = login_user(
            username,
            request["password"],
            users_mutex,
            rate_limiter
        );

        if (result["status"] == "ok") {
            logged_in = true;
            logged_username = username;
            logged_role = result.value("role", "user");

            session_registry.authenticate_session(session_id, logged_username, logged_role);

            log_message(LogLevel::INFO, "login success user=" + username + " role=" + logged_role);
        } else {
            log_message(LogLevel::WARN, "login failed user=" + username + " reason=" + result.value("message", "unknown"));
        }

        return result;
    }

    if (action == "logout") {
        if (!logged_in) {
            return {
                {"status", "error"},
                {"message", "not logged in"}
            };
        }

        log_message(LogLevel::INFO, "logout user=" + logged_username);

        session_registry.clear_session(session_id);

        logged_in = false;
        logged_username.clear();
        logged_role.clear();
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
        json result = list_available_tasks(logged_username, tasks_mutex);
        log_message(LogLevel::INFO, "list available tasks user=" + logged_username);
        return result;
    }

    if (action == "list_active_tasks") {
        json result = list_active_tasks(logged_username, tasks_mutex);
        log_message(LogLevel::INFO, "list active tasks user=" + logged_username);
        return result;
    }

    if (action == "add_task") {
        if (!validate_string_field(request, "task", validation_error)) {
            return validation_error;
        }

        std::string task = request["task"];

        json result = add_task_to_user(logged_username, task, tasks_mutex);

        if (result["status"] == "ok") {
            log_message(LogLevel::INFO, "task added user=" + logged_username + " task=" + task);
        } else {
            log_message(LogLevel::WARN, "task add failed user=" + logged_username + " task=" + task +
                                        " reason=" + result.value("message", "unknown"));
        }

        return result;
    }

    if (action == "remove_task") {
        if (!validate_string_field(request, "task", validation_error)) {
            return validation_error;
        }

        std::string task = request["task"];

        json result = remove_task_from_user(logged_username, task, tasks_mutex);

        if (result["status"] == "ok") {
            log_message(LogLevel::INFO, "task removed user=" + logged_username + " task=" + task);
        } else {
            log_message(LogLevel::WARN, "task remove failed user=" + logged_username + " task=" + task +
                                        " reason=" + result.value("message", "unknown"));
        }

        return result;
    }

    if (action == "execute_task") {
        if (!validate_string_field(request, "task", validation_error)) {
            return validation_error;
        }

        std::string task = request["task"];

        json result = execute_task_for_user(logged_username, task, tasks_mutex);

        if (result["status"] == "ok") {
            log_message(LogLevel::INFO, "task executed user=" + logged_username + " task=" + task);
        } else {
            log_message(LogLevel::WARN, "task execution failed user=" + logged_username + " task=" + task +
                                        " reason=" + result.value("message", "unknown"));
        }

        return result;
    }

    if (action == "list_users") {
        if (logged_role != "admin") {
            return {
                {"status", "error"},
                {"message", "admin only"}
            };
        }

        json result = list_users(users_mutex);
        log_message(LogLevel::INFO, "admin list_users admin=" + logged_username);
        return result;
    }

    if (action == "assign_task") {
        if (logged_role != "admin") {
            return {
                {"status", "error"},
                {"message", "admin only"}
            };
        }

        if (!validate_string_field(request, "target_username", validation_error)) {
            return validation_error;
        }

        if (!validate_string_field(request, "task", validation_error)) {
            return validation_error;
        }

        std::string target = request["target_username"];
        std::string task = request["task"];

        json result = assign_task_to_user(target, task, tasks_mutex);

        if (result["status"] == "ok") {
            log_message(LogLevel::INFO, "admin assign admin=" + logged_username + " target=" + target + " task=" + task);
        } else {
            log_message(LogLevel::WARN, "admin assign failed admin=" + logged_username + " target=" + target +
                                        " task=" + task + " reason=" + result.value("message", "unknown"));
        }

        return result;
    }

    if (action == "kick_user") {
        if (logged_role != "admin") {
            return {
                {"status", "error"},
                {"message", "admin only"}
            };
        }

        if (!validate_string_field(request, "target_username", validation_error)) {
            return validation_error;
        }

        std::string target = request["target_username"];

        if (target == logged_username) {
            return {
                {"status", "error"},
                {"message", "admin cannot kick their own session"}
            };
        }

        int kicked = session_registry.disconnect_user_sessions(target);

        if (kicked > 0) {
            log_message(LogLevel::WARN, "admin kick admin=" + logged_username + " target=" + target +
                                        " disconnected_sessions=" + std::to_string(kicked));
            return {
                {"status", "ok"},
                {"message", "disconnected " + std::to_string(kicked) + " session(s) for user " + target}
            };
        }

        return {
            {"status", "error"},
            {"message", "no active session found for user " + target}
        };
    }

    if (action == "server_status") {
        if (logged_role != "admin") {
            return {
                {"status", "error"},
                {"message", "admin only"}
            };
        }

        json result = {
            {"status", "ok"},
            {"server_status", {
                {"port", config.port},
                {"active_sessions", session_registry.count_active_sessions()},
                {"registered_users", count_registered_users(users_mutex)},
                {"total_active_tasks", count_total_active_tasks(tasks_mutex)},
                {"stop_accepting_new_clients", stop_accepting_new_clients.load()},
                {"shutdown_drain_seconds", config.shutdown_drain_seconds}
            }}
        };

        log_message(LogLevel::INFO, "admin server_status admin=" + logged_username);
        return result;
    }

    if (action == "list_sessions") {
        if (logged_role != "admin") {
            return {
                {"status", "error"},
                {"message", "admin only"}
            };
        }

        json result = {
            {"status", "ok"},
            {"sessions", session_registry.list_active_sessions_json()}
        };

        log_message(LogLevel::INFO, "admin list_sessions admin=" + logged_username);
        return result;
    }

    if (action == "shutdown") {
        if (logged_role != "admin") {
            return {
                {"status", "error"},
                {"message", "admin only"}
            };
        }

        stop_accepting_new_clients = true;

        log_message(LogLevel::WARN, "shutdown initiated by admin=" + logged_username);

        return {
            {"status", "ok"},
            {"message", "shutdown initiated; server stopped accepting new clients"}
        };
    }

    return {
        {"status", "error"},
        {"message", "unknown action"}
    };
}