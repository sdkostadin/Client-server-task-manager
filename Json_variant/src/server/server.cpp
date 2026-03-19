#include <iostream>
#include <thread>
#include <mutex>
#include <cstring>
#include <atomic>
#include <chrono>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <nlohmann/json.hpp>

#include "server/request_handler.hpp"
#include "auth/auth.hpp"
#include "common/logger.hpp"
#include "security/rate_limiter.hpp"
#include "common/config.hpp"
#include "session/session_registry.hpp"

using json = nlohmann::json;

std::mutex users_mutex;
std::mutex tasks_mutex;
std::atomic<bool> g_stop_accepting_new_clients = false;

void handle_client(
    int client_fd,
    RateLimiter& rate_limiter,
    SessionRegistry& session_registry,
    int session_id,
    const ServerConfig& config
) {
    bool logged_in = false;
    std::string logged_username;
    std::string logged_role;

    while (true) {
        char buffer[4096];
        std::memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received < 0) {
            log_message(LogLevel::ERROR, "recv() failed");
            break;
        }

        if (bytes_received == 0) {
            log_message(LogLevel::INFO, "client disconnected");
            break;
        }

        std::string request_text(buffer, bytes_received);

        json response;
        bool should_disconnect = false;

        try {
            json request = json::parse(request_text);

            response = process_request(
                request,
                logged_in,
                logged_username,
                logged_role,
                should_disconnect,
                g_stop_accepting_new_clients,
                users_mutex,
                tasks_mutex,
                rate_limiter,
                session_registry,
                session_id,
                config
            );
        } catch (...) {
            response = {
                {"status", "error"},
                {"message", "invalid json"}
            };

            log_message(LogLevel::WARN, "invalid json received from client");
        }

        std::string response_text = response.dump() + "\n";

        ssize_t bytes_sent = send(
            client_fd,
            response_text.c_str(),
            response_text.size(),
            0
        );

        if (bytes_sent < 0) {
            log_message(LogLevel::ERROR, "send() failed");
            break;
        }

        if (should_disconnect) {
            break;
        }
    }

    session_registry.remove_session(session_id);
    close(client_fd);
}

int main() {
    ServerConfig config = load_config("config/config.json");

    init_logger(config.log_file);
    log_message(LogLevel::INFO, "server starting");

    json admin_init = ensure_admin_exists(users_mutex);
    log_message(LogLevel::INFO, admin_init.value("message", "admin init finished"));

    RateLimiter rate_limiter(config.login_max_attempts, config.login_block_seconds);
    SessionRegistry session_registry;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        log_message(LogLevel::ERROR, "socket() failed");
        return 1;
    }

    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_message(LogLevel::ERROR, "setsockopt() failed");
        close(server_fd);
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config.port);

    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&server_addr),
            sizeof(server_addr)
        ) < 0) {
        log_message(LogLevel::ERROR, "bind() failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, config.backlog) < 0) {
        log_message(LogLevel::ERROR, "listen() failed");
        close(server_fd);
        return 1;
    }

    log_message(LogLevel::INFO, "server listening on port " + std::to_string(config.port));
    std::cout << "Server is listening on port " << config.port << "...\n";

    while (!g_stop_accepting_new_clients.load()) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        timeval tv{};
        tv.tv_sec = config.accept_poll_timeout_ms / 1000;
        tv.tv_usec = (config.accept_poll_timeout_ms % 1000) * 1000;

        int ready = select(server_fd + 1, &readfds, nullptr, nullptr, &tv);

        if (ready < 0) {
            log_message(LogLevel::ERROR, "select() failed");
            break;
        }

        if (ready == 0) {
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(
                server_fd,
                reinterpret_cast<sockaddr*>(&client_addr),
                &client_len
            );

            if (client_fd < 0) {
                log_message(LogLevel::ERROR, "accept() failed");
                continue;
            }

            log_message(LogLevel::INFO, "client connected");

            int session_id = session_registry.create_session(client_fd);

            std::thread client_thread(
                handle_client,
                client_fd,
                std::ref(rate_limiter),
                std::ref(session_registry),
                session_id,
                std::cref(config)
            );

            client_thread.detach();
        }
    }

    if (g_stop_accepting_new_clients.load()) {
        log_message(LogLevel::WARN, "phase 1 complete: stopped accepting new clients");
        std::cout << "Shutdown drain phase started for " << config.shutdown_drain_seconds << " seconds...\n";

        auto drain_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(config.shutdown_drain_seconds);

        while (std::chrono::steady_clock::now() < drain_deadline) {
            if (session_registry.count_active_sessions() == 0) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        log_message(LogLevel::WARN, "phase 2: disconnecting remaining clients");
        session_registry.disconnect_all_sessions("server_shutdown", "server shutting down");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    close(server_fd);

    log_message(LogLevel::WARN, "server shutting down");
    return 0;
}