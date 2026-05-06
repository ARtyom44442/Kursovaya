#include "Transport.h"

class Courier:
public Transport{
private:
    float max_radius;

public:
     Courier(std::string name, float s, float max_w, float max_v, coords pos, float radius);

     bool canHandle(Order& order);

     float getmax_radius(){return max_radius;}
     void setmax_radius(float radius){max_radius = radius;}
     void PrintStats();

};
