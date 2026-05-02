#pragma once

struct coords {
    int x, y;
};

class Order {
private:
    int ID;
    float weight;
    float vol;
    coords destination;
    int max_time;
public:
    Order(int id, float w, float v, coords d, int mt);

    int getID() { return ID; }
    int getWeight() { return weight; }
    int getVol() { return vol; }
    int getMaxTime() { return max_time; }
    coords getDestination() { return destination; }

    void setDestination(coords d);
    void setWeight(float w);
    void setVol(float v);
    void setMaxTime(int mt);

    void PrintStats();
};