#pragma once
#include <string>

class logger {
public:
    static void init(const std::string& filename);
    static void log(const std::string& message);
private:
    static std::string log_file;
    static std::string current_time();
};