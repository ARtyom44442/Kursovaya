#include "Drone.h"
#include <iostream>
#include <string>

Drone::Drone(std::string name, float s, float max_w, float max_v, coords pos, float battery, float deliv_rate, float ch_rate)
    : Transport(name, s, max_w, max_v, pos), battery_life(battery), delivery_rate(deliv_rate), charge_rate(ch_rate) {}

bool Drone::canHandle(Order& order) {
    float total_flight_time = calculateTime(order) + calculateReturnTime(order);
    return Transport::canHandle(order) && (total_flight_time <= battery_life);
}

float Drone::calculatePrice(Order& order) {
    float oneWayTime = calculateTime(order);
    float returnTime = calculateReturnTime(order);
    float totalFlightTime = oneWayTime + returnTime;
    float orderCost = order.getWeight() * 100.0f;
    float wear_cost = 0.004f * orderCost;

    float basePrice = delivery_rate * oneWayTime + charge_rate * totalFlightTime + wear_cost;
    return basePrice * getDemandFactor();
}

float Drone::calculateReturnTime(Order& order) {
    coords currentPos = getCurrentPos();
    float dist = std::sqrt(std::pow(currentPos.x, 2) + std::pow(currentPos.y, 2));
    return dist / getspeed();
}
