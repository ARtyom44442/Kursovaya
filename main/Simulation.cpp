#include "Simulation.h"
#include "logs/logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

SimulationStats simStats;

std::string formatClock(int total_minutes) {
    int h = (total_minutes / 60) % 24;
    int m = total_minutes % 60;
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << h << ":"
       << std::setfill('0') << std::setw(2) << m;
    return ss.str();
}

int getStrategyByCustomerType(int customerType) {
    if (customerType == 1) return 1;
    if (customerType == 2) return 1;
    return 2;
}

std::string getCustomerTypeName(int type) {
    if (type == 1) return "VIP";
    if (type == 2) return "Экспресс-класс";
    return "Эконом-класс";
}

void advanceTime(int minutes, int& v_time, const std::vector<Transport*>& fleet) {
    v_time += minutes;
    std::cout << "\n[Время] Прошло " << minutes << " мин. Текущее время на часах: " << formatClock(v_time) << "\n";
    logger::log("[Симуляция] Время перемотано на " + std::to_string(minutes) + " мин. Текущее виртуальное время: " + formatClock(v_time));
    
    for (Transport* t : fleet) {
        if (t->isBusy() && t->getTimeToFree() <= v_time) {
            t->setBusy(false);
            std::cout << "  >>> Транспорт " << t->getname() << " доставил груз и теперь свободен.\n";
            logger::log("[Автопарк] Транспорт " + t->getname() + " завершил маршрут и освободился.");
        }
    }
}

void performGroupDelivery(const std::vector<Transport*>& fleet, std::vector<Order>& groupOrders, int strategy, int virtual_time) {
    if (groupOrders.empty()) {
        std::cout << "Нет заказов в группе.\n";
        logger::log("[Группа] Ошибка доставки: группа пуста.");
        return;
    }
    
    std::vector<Order> filteredOrders;
    for (const auto& ord : groupOrders) {
        int custType = ord.getCustomerType();
        if (custType == 2 || custType == 3) {
            filteredOrders.push_back(ord);
        } else {
            std::cout << "Заказ ID " << ord.getID() << " (VIP) не может быть в групповой доставке - обрабатывается отдельно.\n";
            logger::log("[Группа] Заказ ID " + std::to_string(ord.getID()) + " (VIP) исключён из групповой доставки.");
        }
    }
    
    if (filteredOrders.empty()) {
        std::cout << "Нет подходящих заказов для групповой доставки.\n";
        logger::log("[Группа] Нет заказов для групповой доставки.");
        return;
    }
    
    std::sort(filteredOrders.begin(), filteredOrders.end(), [](const Order& a, const Order& b) {
        return a.getCustomerType() < b.getCustomerType();
    });
    
    std::cout << "\nПорядок доставки в группе (Экспрес-класс первые, потом Эконом-класс):\n";
    for (const auto& ord : filteredOrders) {
        std::cout << "  ID " << ord.getID() << " - " << getCustomerTypeName(ord.getCustomerType()) << "\n";
    }
    
    if (!Order::areOrdersClose(filteredOrders, 200)) {
        std::cout << "Заказы слишком далеко друг от друга.\n";
        logger::log("[Группа] Ошибка доставки: заказы расположены слишком далеко.");
        return;
    }

    int expressCount = 0, economyCount = 0;
    for (const auto& ord : filteredOrders) {
        if (ord.getCustomerType() == 2) expressCount++;
        else economyCount++;
    }
    int effectiveStrategy = (expressCount >= economyCount) ? 1 : 2;
    
    std::cout << "Выбрана стратегия: " << (effectiveStrategy == 1 ? "Быстрая доставка" : "Экономичная доставка") << "\n";
    logger::log("[Группа] Выбрана стратегия: " + std::string(effectiveStrategy == 1 ? "Быстрая" : "Экономичная"));

    if (effectiveStrategy == 2) {
        for (auto& ord : filteredOrders) {
            ord = Order(ord.getID(), ord.getWeight(), ord.getVol(), ord.getDestination(), ord.getMaxTime() * 2, ord.getCustomerType());
        }
    }

    std::vector<RouteInfo> validRoutes;
    for (Transport* t : fleet) {
        if (t->isBusy()) continue;

        RouteInfo route;
        if (t->buildRoute(filteredOrders, route)) {
            validRoutes.push_back(route);
        }
    }

    if (validRoutes.empty()) {
        std::cout << "\nВнимание: нет доступного транспорта для группы.\n";
        logger::log("[Группа] Ошибка: нет доступного транспорта для выполнения групповой доставки.");
        return;
    }

    RouteInfo best_route = validRoutes[0];
    for (const auto& r : validRoutes) {
        if (effectiveStrategy == 1) {
            if (r.totalTime < best_route.totalTime) best_route = r;
        } else if (effectiveStrategy == 2) {
            if (r.totalPrice < best_route.totalPrice) best_route = r;
        }
    }

    std::cout << "\nРезультат групповой доставки:\n";
    std::cout << "Назначено на " << best_route.transport->getname() << "\n";
    
    float return_time = best_route.returnTime;
    float total_time = best_route.totalTime + return_time;
    
    std::cout << Transport::formatTimeWithReturn(best_route.totalTime, return_time) << "\n";
    std::cout << "Общая стоимость: " << std::fixed << std::setprecision(2) << best_route.totalPrice << " руб.\n";
    
    simStats.ordersDelivered += static_cast<int>(filteredOrders.size());
    simStats.totalProfit += best_route.totalPrice;
    
    std::string order_list = "";
    std::cout << "Порядок доставки (ID заказов): ";
    for (size_t i = 0; i < best_route.orderIndices.size(); ++i) {
        std::cout << filteredOrders[best_route.orderIndices[i]].getID();
        order_list += std::to_string(filteredOrders[best_route.orderIndices[i]].getID()) + " ";
        if (i < best_route.orderIndices.size() - 1) std::cout << " -> ";
    }
    std::cout << "\n";

    best_route.transport->setBusy(true);
    int total_mins = static_cast<int>(total_time * 60);
    best_route.transport->setTimeToFree(virtual_time + total_mins);

    best_route.transport->setPosition({0, 0});
    logger::log("[Группа] Доставка успешно назначена на: " + best_route.transport->getname() + ". ID заказов: " + order_list + "| Общая стоимость: " + std::to_string(best_route.totalPrice) + " руб.");
}

void updateMarketDemand(const std::vector<Transport*>& fleet) {
    if (fleet.empty()) return;

    static float last_factor = 1.0f;
    int busyCount = 0;
    
    for (const Transport* t : fleet) {
        if (t->isBusy()) {
            busyCount++;
        }
    }

    float busyPercentage = static_cast<float>(busyCount) / fleet.size();

    if (busyPercentage > 0.70f) {
        Transport::setDemandFactor(1.30f);
        if (last_factor != 1.30f) {
            std::cout << "\n[Динамический тариф] Высокий спрос! Цены временно повышены на 30%.\n";
            logger::log("[Тариф] Высокий спрос (>70% автопарка занято). Установлен коэффициент 1.30.");
            last_factor = 1.30f;
        }
    } 
    else if (busyPercentage < 0.15f) {
        Transport::setDemandFactor(0.85f); 
        if (last_factor != 0.85f) {
            std::cout << "\n[Динамический тариф] Низкий спрос. Действует скидка 15%!\n";
            logger::log("[Тариф] Низкий спрос (<15% автопарка занято). Установлен коэффициент 0.85.");
            last_factor = 0.85f;
        }
    } 
    else {
        Transport::setDemandFactor(1.0f);
        if (last_factor != 1.0f) {
            logger::log("[Тариф] Спрос стабилизировался. Установлен стандартный коэффициент 1.0.");
            last_factor = 1.0f;
        }
    }
}
