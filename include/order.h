#pragma once

struct coords {
    int x, y;
};

class Order {
private:
    int ID;
    int weight;
    int vol;
    coords destination;
    int max_time;
public:
    Order(int id, int w, int v, coords d, int mt);

    int getID() { return ID; }
    int getWeight() { return weight; }
    int getVol() { return vol; }
    int getMaxTime() { return max_time; }
    coords getDestination() { return destination; }

    void setRandID();
    void setWeight(int w);
    void setVol(int v);
    void setMaxTime(int mt);

    void PrintStats();
};