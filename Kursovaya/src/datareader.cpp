#include "datareader.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<Transport*> DataReader::loadTransports(const std::string& filename) {
    std::vector<Transport*> fleet;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return fleet;
    }

    json j;
    file >> j;

    for (const auto& item : j) {
        std::string type = item["type"];
        std::string name = item["name"];
        float speed = item["speed"];
        float max_w = item["max_weight"];
        float max_v = item["max_vol"];
        coords pos = {item["x"], item["y"]};

        if (type == "drone") {
            float battery = item["battery"];
            fleet.push_back(new Drone(name, speed, max_w, max_v, pos, battery));
        } 
        else if (type == "courier") {
            float radius = item["radius"];
            float hourly_rate = item.value("hourly_rate", 300.0f);
            float base_fee    = item.value("base_fee", 100.0f);
            fleet.push_back(new Courier(name, speed, max_w, max_v, pos, radius, hourly_rate, base_fee));
        }
        else if (type == "truck") {
            fleet.push_back(new Truck(name, speed, max_w, max_v, pos));
        }
    }

    return fleet;
}