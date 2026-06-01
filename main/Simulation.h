#pragma once
#include <vector>
#include <string>
#include "transport/Transport.h"
#include "order/order.h"

struct SimulationStats {
    int totalOrdersGenerated = 0;
    int vipOrders = 0;
    int expressOrders = 0;
    int economyOrders = 0;
    int ordersDelivered = 0;
    int ordersRejected = 0;
    float totalProfit = 0.0f;
};

extern SimulationStats simStats;

std::string formatClock(int total_minutes);
int getStrategyByCustomerType(int customerType);
std::string getCustomerTypeName(int type);

void advanceTime(int minutes, int& v_time, const std::vector<Transport*>& fleet);
void performGroupDelivery(const std::vector<Transport*>& fleet, std::vector<Order>& groupOrders, int strategy, int virtual_time);
void updateMarketDemand(const std::vector<Transport*>& fleet);
