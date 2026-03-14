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

    ssize_t bytes_sent = send(sock, request_text.c_str(), request_text.size(), 0);
    if (bytes_sent < 0) {
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

        std::cout << "Response: " << register_response.dump() << "\n";
    }

    bool logged_in = false;

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

        std::cout << "Response: " << login_response.dump() << "\n";

        if (login_response.contains("status") && login_response["status"] == "ok") {
            logged_in = true;
            std::cout << "You are now logged in.\n";
        } else {
            std::cout << "Wrong credentials. Try again.\n";
        }
    }

    while (true) {
        int choice = 0;
        std::string task;
        json request;

        std::cout << "\n=== Session Menu ===\n";
        std::cout << "1. List available tasks\n";
        std::cout << "2. List active tasks\n";
        std::cout << "3. Add task\n";
        std::cout << "4. Remove task\n";
        std::cout << "5. Execute task\n";
        std::cout << "6. Logout\n";
        std::cout << "Choose option: ";
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 1) {
            request = {
                {"action", "list_available_tasks"}
            };
        } else if (choice == 2) {
            request = {
                {"action", "list_active_tasks"}
            };
        } else if (choice == 3) {
            std::cout << "Enter task name: ";
            std::getline(std::cin, task);

            request = {
                {"action", "add_task"},
                {"task", task}
            };
        } else if (choice == 4) {
            std::cout << "Enter task name: ";
            std::getline(std::cin, task);

            request = {
                {"action", "remove_task"},
                {"task", task}
            };
        } else if (choice == 5) {
            std::cout << "Enter task name: ";
            std::getline(std::cin, task);

            request = {
                {"action", "execute_task"},
                {"task", task}
            };
        } else if (choice == 6) {
            request = {
                {"action", "logout"}
            };
        } else {
            std::cout << "Invalid option\n";
            continue;
        }

        json response;
        if (!send_request_and_get_response(sock, request, response)) {
            break;
        }

        std::cout << "Response: " << response.dump() << "\n";

        if (choice == 6) {
            std::cout << "Session ended.\n";
            break;
        }
    }

    close(sock);
    return 0;
}