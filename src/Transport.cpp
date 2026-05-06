#include "Transport.h"
#include <cmath>
#include <iostream>


Transport::Transport(std::string name, float s, float max_w, float max_v, coords pos): type_name(name), speed(s), max_weight(max_w), max_vol(max_v), current_pos(pos) {}

bool Transport::canHandle(Order& order) {
    return order.getWeight() <= max_weight && order.getVol() <= max_vol;
}

float Transport::calculateTime(Order& order) {
    coords dest = order.getDestination();
    float dist = std::sqrt(std::pow(dest.x - current_pos.x, 2) + std::pow(dest.y - current_pos.y, 2));
    return dist / speed; 
}
