#include "json_utils.hpp"
#include <fstream>

json load_json_or_default(const std::string& filename, const json& default_value) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        return default_value;
    }

    try {
        json data;
        in >> data;
        return data;
    } catch (...) {
        return default_value;
    }
}

bool save_json_to_file(const std::string& filename, const json& data) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        return false;
    }

    out << data.dump(4);
    return true;
}