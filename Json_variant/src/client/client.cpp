#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::mutex g_response_mutex;
std::condition_variable g_response_cv;
std::queue<json> g_response_queue;
std::atomic<bool> g_connection_alive = true;
std::atomic<bool> g_should_exit = false;

void print_string_array(const json& arr) {
    if (!arr.is_array() || arr.empty()) {
        std::cout << "  (empty)\n";
        return;
    }

    for (std::size_t i = 0; i < arr.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << arr[i].get<std::string>() << "\n";
    }
}

void print_users(const json& users) {
    if (!users.is_array() || users.empty()) {
        std::cout << "  (no users)\n";
        return;
    }

    for (std::size_t i = 0; i < users.size(); ++i) {
        std::cout << "  " << (i + 1) << ". "
                  << users[i].value("username", "")
                  << " [" << users[i].value("role", "user") << "]\n";
    }
}

void print_sessions(const json& sessions) {
    if (!sessions.is_array() || sessions.empty()) {
        std::cout << "  (no active sessions)\n";
        return;
    }

    for (std::size_t i = 0; i < sessions.size(); ++i) {
        std::cout << "  " << (i + 1) << ". "
                  << sessions[i].value("username", "")
                  << " [" << sessions[i].value("role", "user") << "] "
                  << "session_id=" << sessions[i].value("session_id", 0)
                  << "\n";
    }
}

void print_server_status(const json& status_obj) {
    if (!status_obj.is_object()) {
        std::cout << "  (invalid server status)\n";
        return;
    }

    std::cout << "Server status:\n";
    std::cout << "  Port                     : " << status_obj.value("port", 0) << "\n";
    std::cout << "  Active sessions          : " << status_obj.value("active_sessions", 0) << "\n";
    std::cout << "  Registered users         : " << status_obj.value("registered_users", 0) << "\n";
    std::cout << "  Total active tasks       : " << status_obj.value("total_active_tasks", 0) << "\n";
    std::cout << "  Stop accepting new users : "
              << (status_obj.value("stop_accepting_new_clients", false) ? "yes" : "no") << "\n";
    std::cout << "  Shutdown drain seconds   : " << status_obj.value("shutdown_drain_seconds", 0) << "\n";
}

void print_response_pretty(const json& response) {
    std::cout << "\n--- Server Reply ---\n";

    std::string status = response.value("status", "unknown");
    std::cout << "Status : " << (status == "ok" ? "SUCCESS" : "ERROR") << "\n";

    if (response.contains("message")) {
        std::cout << "Message: " << response["message"] << "\n";
    }

    if (response.contains("role")) {
        std::cout << "Role   : " << response["role"] << "\n";
    }

    if (response.contains("available_tasks")) {
        std::cout << "Available tasks:\n";
        print_string_array(response["available_tasks"]);
    }

    if (response.contains("active_tasks")) {
        std::cout << "Active tasks:\n";
        print_string_array(response["active_tasks"]);
    }

    if (response.contains("users")) {
        std::cout << "Users:\n";
        print_users(response["users"]);
    }

    if (response.contains("sessions")) {
        std::cout << "Active sessions:\n";
        print_sessions(response["sessions"]);
    }

    if (response.contains("server_status")) {
        print_server_status(response["server_status"]);
    }

    std::cout << "--------------------\n\n";
}

void print_event_pretty(const json& event_msg) {
    std::cout << "\n*** SERVER EVENT ***\n";
    std::cout << event_msg.value("message", "unknown event") << "\n";
    std::cout << "********************\n\n";
}

void receiver_loop(int sock) {
    std::string pending;

    while (g_connection_alive.load()) {
        char buffer[4096];
        std::memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            g_connection_alive = false;
            g_should_exit = true;
            g_response_cv.notify_all();
            std::cout << "\n[Connection closed by server]\n";
            break;
        }

        pending.append(buffer, bytes_received);

        std::size_t pos = 0;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            pending.erase(0, pos + 1);

            if (line.empty()) {
                continue;
            }

            try {
                json msg = json::parse(line);

                if (msg.contains("type") && msg["type"] == "event") {
                    print_event_pretty(msg);
                    if (msg.value("event", "") == "kick" || msg.value("event", "") == "server_shutdown") {
                        g_connection_alive = false;
                        g_should_exit = true;
                        g_response_cv.notify_all();
                    }
                } else {
                    {
                        std::lock_guard<std::mutex> lock(g_response_mutex);
                        g_response_queue.push(msg);
                    }
                    g_response_cv.notify_one();
                }
            } catch (...) {
                std::cout << "\n[Received invalid message from server]\n";
            }
        }
    }
}

bool send_request_and_get_response(int sock, const json& request, json& response_out) {
    if (!g_connection_alive.load()) {
        return false;
    }

    std::string request_text = request.dump() + "\n";

    if (send(sock, request_text.c_str(), request_text.size(), 0) < 0) {
        std::cerr << "send() failed\n";
        return false;
    }

    std::unique_lock<std::mutex> lock(g_response_mutex);
    g_response_cv.wait(lock, [] {
        return !g_response_queue.empty() || !g_connection_alive.load();
    });

    if (!g_response_queue.empty()) {
        response_out = g_response_queue.front();
        g_response_queue.pop();
        return true;
    }

    return false;
}

bool handle_register(int sock) {
    while (g_connection_alive.load()) {
        std::string username;
        std::string password;

        std::cout << "Enter username: ";
        std::getline(std::cin, username);

        if (g_should_exit.load()) return false;

        std::cout << "Enter password: ";
        std::getline(std::cin, password);

        if (g_should_exit.load()) return false;

        json register_request = {
            {"action", "register"},
            {"username", username},
            {"password", password}
        };

        json register_response;
        if (!send_request_and_get_response(sock, register_request, register_response)) {
            return false;
        }

        print_response_pretty(register_response);

        if (register_response.value("status", "") == "ok") {
            return true;
        }

        std::cout << "Register failed. Try again.\n";
    }

    return false;
}

bool handle_login(int sock, std::string& role_out) {
    while (g_connection_alive.load()) {
        std::string username;
        std::string password;

        std::cout << "Login username: ";
        std::getline(std::cin, username);

        if (g_should_exit.load()) return false;

        std::cout << "Login password: ";
        std::getline(std::cin, password);

        if (g_should_exit.load()) return false;

        json login_request = {
            {"action", "login"},
            {"username", username},
            {"password", password}
        };

        json login_response;
        if (!send_request_and_get_response(sock, login_request, login_response)) {
            return false;
        }

        print_response_pretty(login_response);

        if (login_response.value("status", "") == "ok") {
            role_out = login_response.value("role", "user");
            std::cout << "You are now logged in.\n";
            return true;
        }

        std::cout << "Wrong credentials. Try again.\n";
    }

    return false;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "inet_pton() failed\n";
        close(sock);
        return 1;
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "connect() failed\n";
        close(sock);
        return 1;
    }

    std::thread receiver(receiver_loop, sock);

    int start_choice = 0;
    std::cout << "1. Register\n";
    std::cout << "2. Login\n";
    std::cout << "Choose option: ";
    std::cin >> start_choice;
    std::cin.ignore();

    if (start_choice == 1) {
        if (!handle_register(sock)) {
            g_should_exit = true;
            shutdown(sock, SHUT_RDWR);
            if (receiver.joinable()) receiver.join();
            close(sock);
            return 1;
        }
    } else if (start_choice != 2) {
        std::cout << "Invalid option\n";
        g_should_exit = true;
        shutdown(sock, SHUT_RDWR);
        if (receiver.joinable()) receiver.join();
        close(sock);
        return 1;
    }

    std::string role = "user";
    if (!handle_login(sock, role)) {
        g_should_exit = true;
        shutdown(sock, SHUT_RDWR);
        if (receiver.joinable()) receiver.join();
        close(sock);
        return 1;
    }

    while (!g_should_exit.load()) {
        int choice = 0;
        std::string task;
        std::string target_username;
        json request;

        std::cout << "=== Session Menu ===\n";
        std::cout << "1. List available tasks\n";
        std::cout << "2. List active tasks\n";
        std::cout << "3. Add task\n";
        std::cout << "4. Remove task\n";
        std::cout << "5. Execute task\n";

        if (role == "admin") {
            std::cout << "6. List users\n";
            std::cout << "7. List active sessions\n";
            std::cout << "8. Server status\n";
            std::cout << "9. Assign task to user\n";
            std::cout << "10. Kick user\n";
            std::cout << "11. Shutdown server\n";
            std::cout << "12. Logout\n";
        } else {
            std::cout << "6. Logout\n";
        }

        std::cout << "Choose option: ";
        std::cin >> choice;
        std::cin.ignore();

        if (g_should_exit.load()) {
            break;
        }

        if (role == "admin") {
            if (choice == 1) {
                request = {{"action", "list_available_tasks"}};
            } else if (choice == 2) {
                request = {{"action", "list_active_tasks"}};
            } else if (choice == 3) {
                std::cout << "Enter task name: ";
                std::getline(std::cin, task);
                request = {{"action", "add_task"}, {"task", task}};
            } else if (choice == 4) {
                std::cout << "Enter task name: ";
                std::getline(std::cin, task);
                request = {{"action", "remove_task"}, {"task", task}};
            } else if (choice == 5) {
                std::cout << "Enter task name: ";
                std::getline(std::cin, task);
                request = {{"action", "execute_task"}, {"task", task}};
            } else if (choice == 6) {
                request = {{"action", "list_users"}};
            } else if (choice == 7) {
                request = {{"action", "list_sessions"}};
            } else if (choice == 8) {
                request = {{"action", "server_status"}};
            } else if (choice == 9) {
                std::cout << "Enter target username: ";
                std::getline(std::cin, target_username);
                std::cout << "Enter task name: ";
                std::getline(std::cin, task);

                request = {
                    {"action", "assign_task"},
                    {"target_username", target_username},
                    {"task", task}
                };
            } else if (choice == 10) {
                std::cout << "Enter target username: ";
                std::getline(std::cin, target_username);

                request = {
                    {"action", "kick_user"},
                    {"target_username", target_username}
                };
            } else if (choice == 11) {
                request = {{"action", "shutdown"}};
            } else if (choice == 12) {
                request = {{"action", "logout"}};
            } else {
                std::cout << "Invalid option\n";
                continue;
            }
        } else {
            if (choice == 1) {
                request = {{"action", "list_available_tasks"}};
            } else if (choice == 2) {
                request = {{"action", "list_active_tasks"}};
            } else if (choice == 3) {
                std::cout << "Enter task name: ";
                std::getline(std::cin, task);
                request = {{"action", "add_task"}, {"task", task}};
            } else if (choice == 4) {
                std::cout << "Enter task name: ";
                std::getline(std::cin, task);
                request = {{"action", "remove_task"}, {"task", task}};
            } else if (choice == 5) {
                std::cout << "Enter task name: ";
                std::getline(std::cin, task);
                request = {{"action", "execute_task"}, {"task", task}};
            } else if (choice == 6) {
                request = {{"action", "logout"}};
            } else {
                std::cout << "Invalid option\n";
                continue;
            }
        }

        json response;
        if (!send_request_and_get_response(sock, request, response)) {
            break;
        }

        print_response_pretty(response);

        if ((role == "admin" && choice == 11) ||
            (role == "admin" && choice == 12) ||
            (role != "admin" && choice == 6)) {
            std::cout << "Session ended.\n";
            break;
        }
    }

    g_should_exit = true;
    shutdown(sock, SHUT_RDWR);

    if (receiver.joinable()) {
        receiver.join();
    }

    close(sock);
    return 0;
}