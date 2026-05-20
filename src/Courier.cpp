#include "Courier.h"
#include <iostream>
#include <cmath>

Courier::Courier(std::string name, float s, float max_w, float max_v, coords pos, float radius,  float rate, float fee): Transport(name, s, max_w, max_v, pos), max_radius(radius), hourly_rate(rate), base_fee(fee) {}

bool Courier::canHandle(Order& order) {
    if (Transport::canHandle(order) == false) {
        return false;
    }
    coords dest = order.getDestination();
    coords currentPos = getCurrentPos();
    float dist = (std::sqrt(std::pow(dest.x - currentPos.x, 2) + std::pow(dest.y - currentPos.y, 2)));
    return dist <= max_radius;
}
float Courier::calculatePrice(Order& order) {
    float timeHours = calculateTime(order); 
    return base_fee + hourly_rate * timeHours;
}

void Courier::PrintStats() {}
