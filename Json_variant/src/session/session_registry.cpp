#include "session/session_registry.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <string>

int SessionRegistry::create_session(int socket_fd) {
    std::lock_guard<std::mutex> lock(mutex_);

    int session_id = next_session_id_++;
    sessions_[session_id] = SessionInfo{};
    sessions_[session_id].socket_fd = socket_fd;

    return session_id;
}

void SessionRegistry::authenticate_session(int session_id, const std::string& username, const std::string& role) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second.authenticated = true;
        it->second.username = username;
        it->second.role = role;
    }
}

void SessionRegistry::clear_session(int session_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second.authenticated = false;
        it->second.username.clear();
        it->second.role.clear();
    }
}

void SessionRegistry::remove_session(int session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session_id);
}

int SessionRegistry::count_active_sessions() {
    std::lock_guard<std::mutex> lock(mutex_);

    int count = 0;
    for (const auto& [id, session] : sessions_) {
        if (session.authenticated) {
            count++;
        }
    }

    return count;
}

json SessionRegistry::list_active_sessions_json() {
    std::lock_guard<std::mutex> lock(mutex_);

    json arr = json::array();

    for (const auto& [id, session] : sessions_) {
        if (session.authenticated) {
            arr.push_back({
                {"session_id", id},
                {"username", session.username},
                {"role", session.role}
            });
        }
    }

    return arr;
}

int SessionRegistry::disconnect_user_sessions(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);

    int disconnected_count = 0;

    json msg = {
        {"type", "event"},
        {"event", "kick"},
        {"message", "you have been kicked by admin"}
    };

    std::string payload = msg.dump() + "\n";

    for (auto& [id, session] : sessions_) {
        if (session.authenticated && session.username == username && session.socket_fd >= 0) {
            send(session.socket_fd, payload.c_str(), payload.size(), 0);
            shutdown(session.socket_fd, SHUT_RDWR);
            disconnected_count++;
        }
    }

    return disconnected_count;
}

void SessionRegistry::disconnect_all_sessions(const std::string& event_name, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    json msg = {
        {"type", "event"},
        {"event", event_name},
        {"message", message}
    };

    std::string payload = msg.dump() + "\n";

    for (auto& [id, session] : sessions_) {
        if (session.socket_fd >= 0) {
            send(session.socket_fd, payload.c_str(), payload.size(), 0);
            shutdown(session.socket_fd, SHUT_RDWR);
        }
    }
}