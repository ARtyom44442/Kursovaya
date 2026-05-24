#pragma once
#include <string>
#include "order.h"
#include <cmath>
#include <vector>

struct RouteInfo {
    class Transport* transport;
    std::vector<int> orderIndices;
    std::vector<float> arrivalTimes;
    float totalTime;
    float totalPrice;
};

class Transport {
private:
    std::string type_name;
    float speed;
    float max_weight;
    float max_vol;
    coords current_pos;
    bool busy;
    static float demand_factor;
    int time_to_free; 

public:
    Transport(std::string name, float s, float max_w, float max_v, coords pos);
    
    virtual ~Transport() = default;

    virtual bool canHandle(Order& order);
    virtual float calculateTime(Order& order);
    virtual float calculatePrice(Order& order) { return 0.0f; };

    std::string getname() { return type_name; }
    float getspeed() { return speed; }
    coords getCurrentPos() { return current_pos; }
    float getmax_w() const { return max_weight; }
    float getmax_v() const { return max_vol; }
    
    bool isBusy() const { return busy; }
    void setBusy(bool b) { busy = b; }
    void setPosition(coords pos) { current_pos = pos; }

    int getTimeToFree() const { return time_to_free; }
    void setTimeToFree(int t) { time_to_free = t; }

    void set_type_name(std::string name) { type_name = name; }
    void setSpeed(float s) { speed = s; }
    void setmax_w(float w) { max_weight = w; }
    void setmax_v(float v) { max_vol = v; }
    
    static void setDemandFactor(float factor);
    static float getDemandFactor();

    void PrintStats();
    bool buildRoute(const std::vector<Order>& orders, RouteInfo& route);
    static std::string formatTime(float hours);
};