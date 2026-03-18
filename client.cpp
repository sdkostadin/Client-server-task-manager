#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool send_request_and_get_response(int sock, const json& request, json& response_out) {
    std::string request_text = request.dump() + "\n";

    if (send(sock, request_text.c_str(), request_text.size(), 0) < 0) {
        std::cerr << "send() failed\n";
        return false;
    }

    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        std::cerr << "recv() failed\n";
        return false;
    }

    if (bytes_received == 0) {
        std::cerr << "Server closed connection\n";
        return false;
    }

    try {
        response_out = json::parse(std::string(buffer, bytes_received));
    } catch (...) {
        std::cerr << "Failed to parse server response\n";
        return false;
    }

    return true;
}

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

    std::cout << "--------------------\n\n";
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

    int start_choice = 0;
    std::cout << "1. Register\n";
    std::cout << "2. Login\n";
    std::cout << "Choose option: ";
    std::cin >> start_choice;
    std::cin.ignore();

    if (start_choice == 1) {
        std::string username;
        std::string password;

        std::cout << "Enter username: ";
        std::getline(std::cin, username);

        std::cout << "Enter password: ";
        std::getline(std::cin, password);

        json register_request = {
            {"action", "register"},
            {"username", username},
            {"password", password}
        };

        json register_response;
        if (!send_request_and_get_response(sock, register_request, register_response)) {
            close(sock);
            return 1;
        }

        print_response_pretty(register_response);
    }

    bool logged_in = false;
    std::string role = "user";

    while (!logged_in) {
        std::string username;
        std::string password;

        std::cout << "Login username: ";
        std::getline(std::cin, username);

        std::cout << "Login password: ";
        std::getline(std::cin, password);

        json login_request = {
            {"action", "login"},
            {"username", username},
            {"password", password}
        };

        json login_response;
        if (!send_request_and_get_response(sock, login_request, login_response)) {
            close(sock);
            return 1;
        }

        print_response_pretty(login_response);

        if (login_response.value("status", "") == "ok") {
            logged_in = true;
            role = login_response.value("role", "user");
            std::cout << "You are now logged in.\n";
        } else {
            std::cout << "Wrong credentials. Try again.\n";
        }
    }

    while (true) {
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
            std::cout << "7. Assign task to user\n";
            std::cout << "8. Logout\n";
        } else {
            std::cout << "6. Logout\n";
        }

        std::cout << "Choose option: ";
        std::cin >> choice;
        std::cin.ignore();

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
                std::cout << "Enter target username: ";
                std::getline(std::cin, target_username);
                std::cout << "Enter task name: ";
                std::getline(std::cin, task);

                request = {
                    {"action", "assign_task"},
                    {"target_username", target_username},
                    {"task", task}
                };
            } else if (choice == 8) {
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

        if ((role == "admin" && choice == 8) || (role != "admin" && choice == 6)) {
            std::cout << "Session ended.\n";
            break;
        }
    }

    close(sock);
    return 0;
}