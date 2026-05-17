#include <iostream>
#include <vector>
#include <limits>
#include <clocale>
#include <string>
#include <sstream>
//#include <windows.h>
#include <iomanip>
#include <cmath>
#include "order.h"
#include "datareader.h"
#include "Transport.h"
#include <algorithm>   
#include <climits>     
const float PROXIMITY_THRESHOLD = 50.0f;

float getFloatInput(const std::string& prompt) {
    float val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val > 0 && val <= 10000) {
            return val;
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "  [Ошибка] Введите число больше 0 и не более 10000.\n";
    }
}

int getIntInput(const std::string& prompt) {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= 0 && val <= 10000) {
            return val;
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "  [Ошибка] Введите целое число от 0 до 10000.\n";
    }
}

int getStrategyChoice() {
    int strat;
    while (true) {
        std::cout << "\nВЫБЕРИТЕ СТРАТЕГИЮ ДОСТАВКИ:\n";
        std::cout << "1. Быстрая доставка (приоритет времени)\n";
        std::cout << "2. Экономичная доставка (минимальная цена, лимит времени увеличен в 2 раза)\n";
        std::cout << "Выбор: ";
        if (std::cin >> strat && (strat == 1 || strat == 2)) {
            return strat;
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "  [Ошибка] Введите 1 или 2.\n";
    }
}

std::string formatTime(float hours) {
    std::stringstream ss;
    int h = static_cast<int>(hours);
    int m = static_cast<int>((hours - h) * 60 + 0.5f);
    if (m >= 60) {
        h++;
        m = 0;
    }
    if (h > 0) {
        ss << h << " час. ";
    }
    if (m > 0 || h == 0) {
        ss << m << " мин.";
    }
    return ss.str();
}

bool areOrdersClose(const std::vector<Order>& orders, float threshold) {
    if (orders.size() < 2) return true;
    for (size_t i = 0; i < orders.size(); ++i) {
        for (size_t j = i + 1; j < orders.size(); ++j) {
            coords a = orders[i].getDestination();
            coords b = orders[j].getDestination();
            float dist = std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
            if (dist > threshold) return false;
        }
    }
    return true;
}
struct RouteInfo {
    Transport* transport;
    std::vector<int> orderIndices;
    std::vector<float> arrivalTimes;
    float totalTime;
    float totalPrice;
};
bool buildRoute(Transport* t, const std::vector<Order>& orders, RouteInfo& route) {
    float totalWeight = 0, totalVol = 0;
    for (const auto& ord : orders) {
        totalWeight += ord.getWeight();
        totalVol += ord.getVol();
    }
    if (totalWeight > t->getmax_w() || totalVol > t->getmax_v()) return false;

    coords currentPos = t->getCurrentPos();
    std::vector<bool> delivered(orders.size(), false);
    std::vector<int> orderIdx;
    std::vector<float> times;
    float elapsed = 0.0f;
    float total_price = 0.0f;

    for (size_t k = 0; k < orders.size(); ++k) {
        int bestIdx = -1;
        float bestDist = std::numeric_limits<float>::max();
        for (size_t i = 0; i < orders.size(); ++i) {
            if (delivered[i]) continue;
            coords dest = orders[i].getDestination();
            float dist = std::sqrt((dest.x - currentPos.x) * (dest.x - currentPos.x) +
                (dest.y - currentPos.y) * (dest.y - currentPos.y));
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = static_cast<int>(i);
            }
        }
        if (bestIdx == -1) break;

        float travelTime = bestDist / t->getspeed();
        float expectedArrivalTime = elapsed + travelTime;
        float maxTimeHours = orders[bestIdx].getMaxTime() / 60.0f;
        if (expectedArrivalTime > maxTimeHours) {
            return false; 
        }
        elapsed = expectedArrivalTime;
        orderIdx.push_back(bestIdx);
        times.push_back(elapsed);

        total_price += t->calculatePrice(const_cast<Order&>(orders[bestIdx]));

        currentPos = orders[bestIdx].getDestination();
        delivered[bestIdx] = true;
    }

    route.transport = t;
    route.orderIndices = orderIdx;
    route.arrivalTimes = times;
    route.totalTime = elapsed;
    route.totalPrice = total_price;
    return true;
}

void performGroupDelivery(const std::vector<Transport*>& fleet, std::vector<Order>& groupOrders, int strategy) {
    if (groupOrders.empty()) {
        std::cout << "Нет заказов в группе. Сначала добавьте заказы (пункт 3).\n";
        return;
    }

    if (!areOrdersClose(groupOrders, PROXIMITY_THRESHOLD)) {
        std::cout << "Заказы расположены слишком далеко друг от друга (более " << PROXIMITY_THRESHOLD << " км). Группировка невозможна.\n";
        return;
    }

    // Применение стратегии
    if (strategy == 2) {
        std::cout << "\n[Информация] Экономичная стратегия. Лимиты времени всех заказов в группе увеличены в 2 раза.\n";
        for (auto& ord : groupOrders) {
            ord = Order(ord.getID(), ord.getWeight(), ord.getVol(), ord.getDestination(), ord.getMaxTime() * 2);
        }
    }

    std::vector<RouteInfo> validRoutes;
    for (Transport* t : fleet) {
        RouteInfo route;
        if (buildRoute(t, groupOrders, route)) {
            validRoutes.push_back(route);
        }
    }

    if (validRoutes.empty()) {
        std::cout << "Нет подходящего транспорта для группировки этих заказов (проверьте вес, объём или лимиты времени).\n";
        return;
    }

    RouteInfo bestRoute;
    if (strategy == 2) { 
        bestRoute = *std::min_element(validRoutes.begin(), validRoutes.end(),
            [](const RouteInfo& a, const RouteInfo& b) { return a.totalPrice < b.totalPrice; });
    } else {             
        bestRoute = *std::min_element(validRoutes.begin(), validRoutes.end(),
            [](const RouteInfo& a, const RouteInfo& b) { return a.totalTime < b.totalTime; });
    }

    std::cout << "\n=== ЛУЧШИЙ ТРАНСПОРТ (" << (strategy == 1 ? "Приоритет скорости" : "Приоритет цены") << ") ===\n";
    std::cout << "Назначен: " << bestRoute.transport->getname() << "\n";
    std::cout << "Последовательность доставки:\n";
    for (size_t i = 0; i < bestRoute.orderIndices.size(); ++i) {
        int idx = bestRoute.orderIndices[i];
        const Order& ord = groupOrders[idx];
        std::cout << "  " << (i + 1) << ". Заказ #" << ord.getID()
            << " — время прибытия: " << formatTime(bestRoute.arrivalTimes[i]) << "\n";
    }
    std::cout << "Общее время: " << formatTime(bestRoute.totalTime) << "\n";
    std::cout << "Общая стоимость: " << std::fixed << std::setprecision(2) << bestRoute.totalPrice << " руб.\n";
}
bool isLogicalChoice(std::string name, float w, float v, int x, int y) {
    float dist = std::sqrt(static_cast<float>(x * x + y * y));
    bool isTruck = (name.find("Truck") != std::string::npos);
    
    if (isTruck) {
        if (w > 15 || v > 2) {
            return true;
        }
        if (dist < 10) {
            return false;
        }
    }
    return true;
}


int main() {
    std::setlocale(LC_ALL, "ru_RU.UTF-8");
    
    DataReader reader;
    std::vector<Transport*> fleet = reader.loadTransports("data/transports.json");
    std::vector<Order> groupOrders;

    if (fleet.empty()) {
        std::cerr << "Ошибка: Автопарк пуст или файл не найден\n";
        return 1;
    }

    int choice = -1;
    int order_counter = 1;

    while (true) {
        std::cout << "\nГЛАВНОЕ МЕНЮ\n";
        std::cout << "1. Создать одиночный заказ\n";
        std::cout << "2. Посмотреть доступный транспорт\n";
        std::cout << "3. Добавить заказ в группу\n";
        std::cout << "4. Выполнить групповую доставку\n";
        std::cout << "0. Выход\n";
        std::cout << "Выбор: ";
        
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Неверный ввод меню\n";
            continue;
        }

        if (choice == 0) break;

        if (choice == 2) {
            std::cout << "\nДОСТУПНЫЙ АВТОПАРК\n";
            for (Transport* t : fleet) {
                std::cout << " - " << t->getname() << " (Скорость: " << t->getspeed() << " км/ч)\n";
            }
        }
        else if (choice == 3) {

            std::cout << "\nОФОРМЛЕНИЕ ЗАКАЗА ДЛЯ ГРУППЫ\n";
            std::cout << "Вес (кг): ";
            float w = getFloatInput("Вес (кг): ");
            float v = getFloatInput("Объем (м3): ");
            float max_time = getFloatInput("Макс. время доставки (часы): ");
            int x = getIntInput("Координата X: ");
            int y = getIntInput("Координата Y: ");

            coords dest = { x, y };
            int max_time_minutes = static_cast<int>(max_time * 60);
            Order newOrder(order_counter, w, v, dest, max_time_minutes);
            groupOrders.push_back(newOrder);
            std::cout << "Заказ #" << order_counter << " добавлен в группу. Всего заказов в группе: " << groupOrders.size() << "\n";
            order_counter++;
        }
        else if (choice == 4) {
           if (groupOrders.empty()) {
                std::cout << "Группа пуста. Сначала добавьте заказы через пункт 3.\n";
                continue;
            }
            int strategy = getStrategyChoice();
            performGroupDelivery(fleet, groupOrders, strategy);
            groupOrders.clear();
        }
       else if (choice == 1) {
            std::cout << "\n=== ОФОРМЛЕНИЕ ОДИНОЧНОГО ЗАКАЗА ===\n";
            float w = getFloatInput("Вес (кг): ");
            float v = getFloatInput("Объем (м3): ");
            float max_time = getFloatInput("Макс. время доставки (часы): ");
            int x = getIntInput("Координата X: ");
            int y = getIntInput("Координата Y: ");

            coords dest = { x, y };
            
            int strat_input = getStrategyChoice();

            if (strat_input == 2) {
                max_time *= 2.0f; 
                std::cout << "\n[Информация] Экономичная стратегия. Лимит времени ожидания увеличен до " << max_time << " часов.\n";
            }

            Order newOrder(order_counter, w, v, dest, static_cast<int>(max_time * 60));

            std::cout << "\nРАСПРЕДЕЛЕНИЕ\n";
            
            Transport* best_transport = nullptr;
            float best_metric = std::numeric_limits<float>::max(); 
            float best_final_time = 0.0f;
            float best_final_price = 0.0f;

            for (Transport* t : fleet) {
                if (!isLogicalChoice(t->getname(), w, v, x, y)) {
                    std::cout << t->getname() << " не подходит (нерентабельно)\n";
                    continue;
                }

                if(!t->canHandle(newOrder)){
                    std::cout << t->getname() << " не подходит (ограничения веса/объема)\n";
                    continue;
                }
                
                float current_time = t->calculateTime(newOrder);
                if (current_time > max_time) {
                    std::cout << t->getname() << " не успеет (нужно: "
                              << formatTime(current_time) << ", жесткий лимит: " << max_time << " ч)\n";
                    continue;
                }
                
                float current_price = t->calculatePrice(newOrder);

                std::cout << t->getname() << " справится за " << formatTime(current_time)
                          << " (Цена: " << std::fixed << std::setprecision(2)
                          << current_price << " руб.)\n";
                
                if (strat_input == 1) { 
                    if (current_time < best_metric) {
                        best_metric = current_time;
                        best_transport = t;
                        best_final_time = current_time;
                        best_final_price = current_price;
                    }
                } else if (strat_input == 2) { 
                    if (current_price < best_metric) {
                        best_metric = current_price;
                        best_transport = t;
                        best_final_time = current_time;
                        best_final_price = current_price;
                    }
                }
            }

            if (best_transport != nullptr) {
                std::cout << "\nРЕЗУЛЬТАТ (" << (strat_input == 1 ? "Быстрая стратегия" : "Самая дешевая стратегия") << "):\n";
                std::cout << "Назначено на " << best_transport->getname() 
                          << " (Время: " << formatTime(best_final_time) << ")\n";
                std::cout << "Стоимость доставки: " << std::fixed << std::setprecision(2) << best_final_price << " руб.\n";
            } else {
                std::cout << "\nВНИМАНИЕ: Нет доступного транспорта для этого заказа даже с учетом стратегии.\n";
            }
            order_counter++; 
        }
        else {
            std::cout << "Неверный выбор меню\n";
        }
    }

    for (Transport* t : fleet) {
        delete t;
    }
    fleet.clear();

    return 0;
}
