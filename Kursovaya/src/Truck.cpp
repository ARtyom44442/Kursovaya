#include "Truck.h"
#include <cmath>

Truck::Truck(std::string name, float s, float max_w, float max_v, coords pos, float deliv_rate, float f_rate) 
    : Transport(name, s, max_w, max_v, pos), delivery_rate(deliv_rate), fuel_rate(f_rate) {
    traffic_factor_min = 1.0;
    traffic_factor_max = 1.5;
}

void Truck::settrafficfactorrange(float min, float max) {
    if (min <= max) {
        traffic_factor_min = min;
        traffic_factor_max = max;
    }
}

float Truck::getRandomTrafficFactor() {
    float fraction = (float)rand() / RAND_MAX;
    return traffic_factor_min + fraction * (traffic_factor_max - traffic_factor_min);
}

float Truck::calculateTime(Order& order) {
    coords dest = order.getDestination();
    coords currentPos = getCurrentPos();

    float dist = (std::sqrt(std::pow(dest.x - currentPos.x, 2) + std::pow(dest.y - currentPos.y, 2)));
    float speed = getspeed();
    if (speed <= 0) return 0;
    float baseTime = dist / speed;
    return baseTime * getRandomTrafficFactor();
}

float Truck::calculatePrice(Order& order){
    float timeHours = calculateTime(order);
    return (delivery_rate * timeHours + fuel_rate * timeHours * 2) * 1.008;
}