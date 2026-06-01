#include "Transport.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <limits>
#include <iomanip>
#include <string>

Transport::Transport(std::string name, float s, float max_w, float max_v, coords pos)
    : type_name(name), speed(s), max_weight(max_w), max_vol(max_v), current_pos(pos), busy(false), time_to_free(0) {}

bool Transport::canHandle(Order& order) {
    return order.getWeight() <= max_weight && order.getVol() <= max_vol;
}

float Transport::demand_factor = 1.0f;

void Transport::setDemandFactor(float factor) {
    if (factor > 0) {
        demand_factor = factor;
    }
}

float Transport::getDemandFactor() {
    return demand_factor;
}

float Transport::calculateTime(Order& order) {
    coords dest = order.getDestination();
    float dist = std::sqrt(std::pow(dest.x - current_pos.x, 2) + std::pow(dest.y - current_pos.y, 2));
    return dist / speed;
}

float Transport::calculateReturnTime(Order& order) {
    float dist = std::sqrt(std::pow(current_pos.x, 2) + std::pow(current_pos.y, 2));
    return dist / speed;
}

std::string Transport::formatTime(float hours) {
    std::stringstream ss;
    int h = static_cast<int>(hours);
    int m = static_cast<int>((hours - h) * 60 + 0.5f);
    if (m >= 60) { h++; m = 0; }
    if (h > 0) ss << h << " час. ";
    if (m > 0 || h == 0) ss << m << " мин.";
    return ss.str();
}

std::string Transport::formatTimeWithReturn(float hours, float returnHours) {
    std::stringstream ss;
    int h = static_cast<int>(hours);
    int m = static_cast<int>((hours - h) * 60 + 0.5f);
    if (m >= 60) { h++; m = 0; }
    
    float totalTime = hours + returnHours;
    int totalH = static_cast<int>(totalTime);
    int totalM = static_cast<int>((totalTime - totalH) * 60 + 0.5f);
    if (totalM >= 60) { totalH++; totalM = 0; }
    
    ss << "Доставка: ";
    if (h > 0) ss << h << " час. ";
    if (m > 0 || h == 0) ss << m << " мин.";
    
    ss << " | Вернётся: ";
    if (totalH > 0) ss << totalH << " час. ";
    if (totalM > 0 || totalH == 0) ss << totalM << " мин.";
    
    return ss.str();
}

bool Transport::buildRoute(const std::vector<Order>& orders, RouteInfo& route) {
    float totalWeight = 0, totalVol = 0;
    for (const auto& ord : orders) {
        totalWeight += ord.getWeight();
        totalVol += ord.getVol();
    }
    
    if (totalWeight > max_weight || totalVol > max_vol) return false;

    coords currentPos = getCurrentPos();
    std::vector<bool> delivered(orders.size(), false);
    std::vector<int> orderIdx;
    std::vector<float> times;
    float elapsed = 0.0f;
    float total_price = 0.0f;

    for (size_t k = 0; k < orders.size(); ++k) {
        int bestIdx = -1;
        float bestDist = std::numeric_limits<float>::max();
        int bestPriority = 4;
        
        for (size_t i = 0; i < orders.size(); ++i) {
            if (delivered[i]) continue;
            
            int currentPriority = orders[i].getCustomerType(); 
            coords dest = orders[i].getDestination();
            float dist = std::sqrt((dest.x - currentPos.x) * (dest.x - currentPos.x) +
                                   (dest.y - currentPos.y) * (dest.y - currentPos.y));

            bool isBetterPriority = (currentPriority < bestPriority);
            bool isSamePriorityCloser = (currentPriority == bestPriority && dist < bestDist);
            
            if (bestIdx == -1 || isBetterPriority || isSamePriorityCloser) {
                bestDist = dist;
                bestIdx = static_cast<int>(i);
                bestPriority = currentPriority;
            }
        }
        
        if (bestIdx == -1) break;

        float travelTime = bestDist / getspeed();
        float expectedArrivalTime = elapsed + travelTime;
        if (expectedArrivalTime > (orders[bestIdx].getMaxTime() / 60.0f)) return false; 

        elapsed = expectedArrivalTime;
        orderIdx.push_back(bestIdx);
        times.push_back(elapsed);
       
        total_price += calculatePrice(const_cast<Order&>(orders[bestIdx]));
        currentPos = orders[bestIdx].getDestination();
        delivered[bestIdx] = true;
    }

    if (orderIdx.size() == orders.size()) {
        route.transport = this;
        route.orderIndices = orderIdx;
        route.arrivalTimes = times;
        route.totalTime = elapsed;
        
        if (!orderIdx.empty()) {
            coords lastPos = orders[orderIdx.back()].getDestination();
            float returnDist = std::sqrt(std::pow(lastPos.x, 2) + std::pow(lastPos.y, 2));
            route.returnTime = returnDist / getspeed();
        } else {
            route.returnTime = 0.0f;
        }
        
        route.totalPrice = total_price;
        return true;
    }
    return false;
}