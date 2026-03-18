#include "common/json_utils.hpp"
#include <fstream>
#include <cstdio>

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

bool save_json_to_file_atomic(const std::string& filename, const json& data) {
    std::string temp_filename = filename + ".tmp";

    {
        std::ofstream out(temp_filename);
        if (!out.is_open()) {
            return false;
        }

        out << data.dump(4);

        if (!out.good()) {
            return false;
        }
    }

    if (std::rename(temp_filename.c_str(), filename.c_str()) != 0) {
        std::remove(temp_filename.c_str());
        return false;
    }

    return true;
}