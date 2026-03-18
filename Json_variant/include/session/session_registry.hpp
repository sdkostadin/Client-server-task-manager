#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SessionRegistry {
public:
    int create_session(int socket_fd);

    void authenticate_session(int session_id, const std::string& username, const std::string& role);
    void clear_session(int session_id);
    void remove_session(int session_id);

    int count_active_sessions();
    json list_active_sessions_json();

    int disconnect_user_sessions(const std::string& username);
    void disconnect_all_sessions(const std::string& event_name, const std::string& message);

private:
    struct SessionInfo {
        int socket_fd = -1;
        bool authenticated = false;
        std::string username;
        std::string role;
    };

    std::unordered_map<int, SessionInfo> sessions_;
    std::mutex mutex_;
    int next_session_id_ = 1;
};