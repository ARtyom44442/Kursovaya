#include "logger.h"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>

std::string logger::log_file = "logs/logs.txt";

void logger::init(const std::string& filename) {
    log_file = filename;
    std::filesystem::path p(log_file);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    std::ofstream file(log_file, std::ios::app);
    if (file.is_open()) {
        file << "\n[" << current_time() << "] Запуск системы логистики\n";
    }
}

void logger::log(const std::string& message) {
    std::ofstream file(log_file, std::ios::app);
    if (file.is_open()) {
        file << "[" << current_time() << "] " << message << "\n";
    }
}

std::string logger::current_time() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << now->tm_mday << "."
       << std::setfill('0') << std::setw(2) << (now->tm_mon + 1) << "."
       << (now->tm_year + 1900) << " "
       << std::setfill('0') << std::setw(2) << now->tm_hour << ":"
       << std::setfill('0') << std::setw(2) << now->tm_min << ":"
       << std::setfill('0') << std::setw(2) << now->tm_sec;
    return ss.str();
}