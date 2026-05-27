#include "Transport.h"

class Drone : public Transport{
private:
    float battery_life;
    float delivery_rate;  
    float charge_rate; 
public:
    Drone(std::string name, float s, float max_w, float max_v, coords pos, float battery,float deliv_rate, float ch_rate);

    bool canHandle(Order& order);
    float calculatePrice(Order& order) override;

    float getBattery() {return battery_life;}
    void setBattery(float b) {battery_life = b;}
    void PrintStats(); 
};
