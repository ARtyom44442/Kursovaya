#pragma once
#include <string>
#include "order.h"
#include <cmath>

class Transport {
private:
    std::string type_name;
    float speed;
    float max_weight;
    float max_vol;
    coords current_pos;

public:
    Transport(std::string name, float s, float max_w, float max_v, coords pos);
    
    virtual ~Transport() = default;

    virtual bool canHandle(Order& order);
    virtual float calculateTime(Order& order);

    std::string getname() { return type_name; }
    float getspeed() { return speed; }
    coords getCurrentPos() { return current_pos; }

    void set_type_name(std::string name) { type_name = name; }
    void setSpeed(float s) { speed = s; }
    void setmax_w(float w) { max_weight = w; }
    void setmax_v(float v) { max_vol = v; }

    void PrintStats();
};
