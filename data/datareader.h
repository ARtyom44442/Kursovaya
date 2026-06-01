#pragma once
#include <vector>
#include <string>
#include "transport/Transport.h"
#include "transport/Drone.h"
#include "transport/Courier.h"
#include "transport/Truck.h"

class DataReader {
public:
    std::vector<Transport*> loadTransports(const std::string& filename);
};