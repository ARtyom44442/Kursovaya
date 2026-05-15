#pragma once
#include "Transport.h"
#include <cstdlib>
#include <ctime>

class Truck : public Transport {
private:
    float traffic_factor_min = 1.0; 
    float traffic_factor_max = 1.5;
public:
    Truck(std::string name, float s, float max_w, float max_v, coords pos);
    float calculateTime(Order& order);
    float gettraffic_factor_min() { return traffic_factor_min; }
    float gettraffic_factor_max() { return traffic_factor_max; }
    void settrafficfactorrange(float min, float max);
  
    void PrintStats();
private:
    float getRandomTrafficFactor();
};