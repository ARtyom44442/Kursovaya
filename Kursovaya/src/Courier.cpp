#include "Courier.h"
#include <iostream>
#include <cmath>

Courier::Courier(std::string name, float s, float max_w, float max_v, coords pos, float radius): Transport(name, s, max_w, max_v, pos), max_radius(radius) {}

bool Courier::canHandle(Order& order) {
    if (Transport::canHandle(order) == false) {
        return false;
    }
    coords dest = order.getDestination();
    coords currentPos = getCurrentPos();
    float dist = (std::sqrt(std::pow(dest.x - currentPos.x, 2) + std::pow(dest.y - currentPos.y, 2)));
    return dist <= max_radius;
}

void Courier::PrintStats() {}
