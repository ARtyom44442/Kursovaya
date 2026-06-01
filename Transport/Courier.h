#include "Transport.h"

class Courier:
public Transport{
private:
    float max_radius;
    float hourly_rate;
    float base_fee;

public:
    Courier(std::string name, float s, float max_w, float max_v, coords pos, float radius, float rate, float fee);

    bool canHandle(Order& order);
    float calculatePrice(Order& order) override;
    float calculateReturnTime(Order& order) override;

    float getmax_radius(){return max_radius;}
    void setmax_radius(float radius){max_radius = radius;}
    void PrintStats();

};
