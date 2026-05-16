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

    std::string getname() const { return type_name; }
    float getspeed() const { return speed; }
    coords getCurrentPos() const { return current_pos; }

    float getmax_w() const { return max_weight; }
    float getmax_v() const { return max_vol; }

    void set_type_name(std::string name) { type_name = name; }
    void setSpeed(float s) { speed = s; }
    void setmax_w(float w) { max_weight = w; }
    void setmax_v(float v) { max_vol = v; }

    void PrintStats();
};