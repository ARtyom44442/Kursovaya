#include "Drone.h"
#include <iostream>


Drone::Drone(std::string name, float s, float max_w, float max_v, coords pos, float battery)
    : Transport(name, s, max_w, max_v, pos), battery_life(battery) {}

bool Drone::canHandle(Order& order) {
    float time_need = calculateTime(order);
    return Transport::canHandle(order) && (time_need * 2 <= battery_life);
}
