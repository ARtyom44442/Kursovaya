#pragma once
#include <vector>
#include "transport/Transport.h"
#include "order/order.h"

struct AppState {
    std::vector<Order> groupOrders;
    int order_counter = 1;
    int virtual_time = 480;
};

void handleSingleOrder(std::vector<Transport*>& fleet, AppState& state);
void handleViewFleet(const std::vector<Transport*>& fleet);
void handleAddToGroup(std::vector<Order>& groupOrders, int& order_counter);
void handleGroupDelivery(std::vector<Transport*>& fleet, std::vector<Order>& groupOrders, int& virtual_time);
void handleAdvanceTime(std::vector<Transport*>& fleet, int& virtual_time);
void handleHalfDaySimulation(std::vector<Transport*>& fleet, AppState& state);
void handleManageGroup(std::vector<Order>& groupOrders);
