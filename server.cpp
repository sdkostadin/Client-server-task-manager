#include <iostream>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>

#include "request_handler.hpp"
#include "auth.hpp"

using json = nlohmann::json;

std::mutex users_mutex;
std::mutex tasks_mutex;

void handle_client(int client_fd) {
    bool logged_in = false;
    std::string logged_username;
    std::string logged_role;

    while (true) {
        char buffer[4096];
        std::memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            std::cerr << "recv() failed\n";
            break;
        }

        if (bytes_received == 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::string request_text(buffer, bytes_received);
        std::cout << "Raw request: " << request_text << "\n";

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
                users_mutex,
                tasks_mutex
            );
        } catch (const std::exception&) {
            response = {
                {"status", "error"},
                {"message", "invalid json"}
            };
        }

        std::string response_text = response.dump() + "\n";

        ssize_t bytes_sent = send(client_fd, response_text.c_str(), response_text.size(), 0);
        if (bytes_sent < 0) {
            std::cerr << "send() failed\n";
            break;
        }

        if (should_disconnect) {
            break;
        }
    }

    close(client_fd);
}

int main() {
    json admin_init = ensure_admin_exists(users_mutex);
    std::cout << admin_init["message"] << "\n";

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt() failed\n";
        close(server_fd);
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "bind() failed\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 16) < 0) {
        std::cerr << "listen() failed\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Server is listening on port 8080...\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            std::cerr << "accept() failed\n";
            continue;
        }

        std::cout << "Client connected successfully.\n";

        std::thread client_thread(handle_client, client_fd);
        client_thread.detach();
    }

    close(server_fd);
    return 0;
}