#include "Drone.h"
#include <iostream>


Drone::Drone(std::string name, float s, float max_w, float max_v, coords pos, float battery, float deliv_rate, float ch_rate)
    : Transport(name, s, max_w, max_v, pos), battery_life(battery), delivery_rate(deliv_rate), charge_rate(ch_rate) {}

bool Drone::canHandle(Order& order) {
    float time_need = calculateTime(order);
    return Transport::canHandle(order) && (time_need * 2 <= battery_life);
}

float Drone::calculatePrice(Order& order) {
    float oneWayTime = calculateTime(order);        
    float totalFlightTime = oneWayTime * 2;         
    float orderCost = order.getWeight() * 100.0f;   
    float wear_cost = 0.002f * orderCost;

    return delivery_rate * oneWayTime + charge_rate * totalFlightTime + wear_cost;
}
