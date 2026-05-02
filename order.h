#pragma once
#include <string>
#include "C:\Users\HanJul\Downloads\Telegram Desktop\ord.h"
#include <cmath>

struct coords {
    int x, y;
};

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

    bool canHandle(Order& order);
    float calculateTime(Order& order);

    std::string getname() { return type_name; }
    float getspeed() { return speed; }

    void set_type_name(std::string name) { type_name = name; }
    void setSpeed(float s) { speed = s; }
    void setmax_w(float w) { max_weight = w; }
    void setmax_v(float v) { max_vol = v; }

    void PrintStats();
};
