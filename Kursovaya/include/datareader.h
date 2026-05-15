#pragma once
#include <vector>
#include <string>
#include "Transport.h"
#include "Drone.h"
#include "Courier.h"
#include "Truck.h"

class DataReader {
public:
    std::vector<Transport*> loadTransports(const std::string& filename);
};