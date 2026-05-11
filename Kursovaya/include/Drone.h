#include "Transport.h"

class Drone : public Transport{
private:
    float battery_life;
public:
    Drone(std::string name, float s, float max_w, float max_v, coords pos, float battery);

    bool canHandle(Order& order);

    float getBattery() {return battery_life;}
    void setBattery(float b) {battery_life = b;}
    void PrintStats(); 
};
