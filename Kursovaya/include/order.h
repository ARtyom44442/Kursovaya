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

    int getID() const { return ID; }
    float getWeight() const { return weight; }
    float getVol() const { return vol; }
    int getMaxTime() const { return max_time; }
    coords getDestination() const { return destination; }

    void setDestination(coords d);
    void setWeight(float w);
    void setVol(float v);
    void setMaxTime(int mt);

    void PrintStats();
};