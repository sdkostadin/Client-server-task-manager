#include "task_executor.hpp"
#include <iostream>
#include <unordered_map>
#include <functional>

namespace {

json read_reports_task(const std::string& username) {
    std::string msg = username + " executed task read_reports";
    std::cout << msg << "\n";
    return {
        {"status", "ok"},
        {"message", msg}
    };
}

json create_invoice_task(const std::string& username) {
    std::string msg = username + " executed task create_invoice";
    std::cout << msg << "\n";
    return {
        {"status", "ok"},
        {"message", msg}
    };
}

json approve_request_task(const std::string& username) {
    std::string msg = username + " executed task approve_request";
    std::cout << msg << "\n";
    return {
        {"status", "ok"},
        {"message", msg}
    };
}

using TaskHandler = std::function<json(const std::string&)>;

const std::unordered_map<std::string, TaskHandler>& get_task_handlers() {
    static const std::unordered_map<std::string, TaskHandler> handlers = {
        {"read_reports", read_reports_task},
        {"create_invoice", create_invoice_task},
        {"approve_request", approve_request_task}
    };

    return handlers;
}

} // namespace

json run_task_handler(const std::string& username, const std::string& task_name) {
    const auto& handlers = get_task_handlers();
    auto it = handlers.find(task_name);

    if (it == handlers.end()) {
        return {
            {"status", "error"},
            {"message", "no handler for task " + task_name}
        };
    }

    return it->second(username);
}