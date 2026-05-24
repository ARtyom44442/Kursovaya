#include <iostream>
#include <vector>
#include <limits>
#include <clocale>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include "order.h"
#include "datareader.h"
#include "Transport.h"
#include <algorithm>   
#include <climits>     
#include "logger.h"

std::string formatClock(int total_minutes) {
    int h = (total_minutes / 60) % 24;
    int m = total_minutes % 60;
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << h << ":"
       << std::setfill('0') << std::setw(2) << m;
    return ss.str();
}

void advanceTime(int minutes, int& v_time, const std::vector<Transport*>& fleet) {
    v_time += minutes;
    std::cout << "\n[Время] Прошло " << minutes << " мин. Текущее время на часах: " << formatClock(v_time) << "\n";
    for (Transport* t : fleet) {
        if (t->isBusy() && t->getTimeToFree() <= v_time) {
            t->setBusy(false);
            std::cout << "  >>> Транспорт " << t->getname() << " доставил груз и теперь свободен.\n";
            logger::log("Транспорт " + t->getname() + " завершил маршрут и освободился.");
        }
    }
}

void performGroupDelivery(const std::vector<Transport*>& fleet, std::vector<Order>& groupOrders, int strategy, int virtual_time) {
    if (groupOrders.empty()) {
        std::cout << "Нет заказов в группе.\n";
        return;
    }
    if (!Order::areOrdersClose(groupOrders, 200)) {
        std::cout << "Заказы слишком далеко друг от друга.\n";
        return;
    }

    if (strategy == 2) {
        for (auto& ord : groupOrders) {
            ord = Order(ord.getID(), ord.getWeight(), ord.getVol(), ord.getDestination(), ord.getMaxTime() * 2, ord.getCustomerType());
        }
    }

    std::vector<RouteInfo> validRoutes;
    for (Transport* t : fleet) {
        if (t->isBusy()) continue;

        RouteInfo route;
        if (t->buildRoute(groupOrders, route)) {
            validRoutes.push_back(route);
        }
    }

    if (validRoutes.empty()) {
        std::cout << "\nВнимание: нет доступного транспорта для группы.\n";
        return;
    }

    RouteInfo best_route = validRoutes[0];
    for (const auto& r : validRoutes) {
        if (strategy == 1) {
            if (r.totalTime < best_route.totalTime) best_route = r;
        } else if (strategy == 2) {
            if (r.totalPrice < best_route.totalPrice) best_route = r;
        }
    }

    std::cout << "\nРезультат групповой доставки:\n";
    std::cout << "Назначено на " << best_route.transport->getname() << "\n";
    std::cout << "Общее время: " << Transport::formatTime(best_route.totalTime) << "\n";
    std::cout << "Общая стоимость: " << std::fixed << std::setprecision(2) << best_route.totalPrice << " руб.\n";
    
    std::cout << "Порядок доставки (ID заказов): ";
    for (size_t i = 0; i < best_route.orderIndices.size(); ++i) {
        std::cout << groupOrders[best_route.orderIndices[i]].getID();
        if (i < best_route.orderIndices.size() - 1) std::cout << " -> ";
    }
    std::cout << "\n";

    best_route.transport->setBusy(true);
    int delivery_time_mins = static_cast<int>(best_route.totalTime * 60);
    best_route.transport->setTimeToFree(virtual_time + delivery_time_mins);

    if (!best_route.orderIndices.empty()) {
        best_route.transport->setPosition(groupOrders[best_route.orderIndices.back()].getDestination());
    }
    logger::log("Групповая доставка назначена на транспорт: " + best_route.transport->getname());
}

void updateMarketDemand(const std::vector<Transport*>& fleet) {
    if (fleet.empty()) return;

    int busyCount = 0;
    for (const Transport* t : fleet) {
        if (t->isBusy()) {
            busyCount++;
        }
    }

    float busyPercentage = static_cast<float>(busyCount) / fleet.size();

    if (busyPercentage > 0.70f) {
        Transport::setDemandFactor(1.30f);
        std::cout << "\n[Динамический тариф] Высокий спрос! Цены временно повышены на 30%.\n";
    } 
    else if (busyPercentage < 0.15f) {
        Transport::setDemandFactor(0.85f); 
        std::cout << "\n[Динамический тариф] Низкий спрос. Действует скидка 15%!\n";
    } 
    else {
        Transport::setDemandFactor(1.0f);  
    }
}

int main() {
    std::setlocale(LC_ALL, "ru_RU.UTF-8");
    logger::init("logs/logs.txt");

    DataReader reader;
    std::vector<Transport*> fleet = reader.loadTransports("data/transports.json");

    if (fleet.empty()) {
        std::cerr << "Ошибка: автопарк пуст\n";
        return 1;
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    for (Transport* t : fleet) t->setPosition({0, 0});
    

    std::vector<Order> groupOrders;
    int choice = -1;
    int order_counter = 1;
    
    int virtual_time = 480; 

    while (true) {
        std::cout << "\nГлавное меню | Время в симуляции: " << formatClock(virtual_time) << "\n";
        std::cout << "1. Создать одиночный заказ\n";
        std::cout << "2. Посмотреть состояние автопарка\n";
        std::cout << "3. Добавить заказ в группу\n";
        std::cout << "4. Выполнить групповую доставку\n";
        std::cout << "5. Перемотать время симуляции (ожидание)\n";
        std::cout << "6. Авто-симуляция (генератор заказов)\n";
        std::cout << "7. Управление текущей группой заказов (Просмотр/Удаление)\n";
        std::cout << "0. Выход\n";
        std::cout << "Выбор: ";

        std::cin >> choice;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        if (choice == 0) break;
        else if (choice == 2) {
            std::cout << "\nСостояние автопарка\n";
            for (Transport* t : fleet) {
                std::string status = t->isBusy() ? "[В рейсе]" : "[Свободен]";
                std::cout << status << " " << t->getname();
                if (t->isBusy()) {
                    std::cout << " (освободится в " << formatClock(t->getTimeToFree()) << ")";
                }
                std::cout << "\n";
            }
        }
        else if (choice == 5) {
            int skip = UI::getIntInput("Сколько минут пропустить? ");
            advanceTime(skip, virtual_time, fleet);
        }
        else if (choice == 6) {
            int sim_count = UI::getIntInput("Сколько заказов сгенерировать? ");
            for (int i = 0; i < sim_count; ++i) {
                int jump = (std::rand() % 26) + 5; 
                advanceTime(jump, virtual_time, fleet);
                updateMarketDemand(fleet);

                float w = (std::rand() % 150) / 10.0f + 1.0f; 
                float v = (std::rand() % 30) / 10.0f + 0.1f; 
                int max_t = (std::rand() % 120) + 40; 
                int x = (std::rand() % 201) - 100; 
                int y = (std::rand() % 201) - 100;
                int cust_type = (std::rand() % 3) + 1;

                Order autoOrder(order_counter, w, v, {x, y}, max_t, cust_type);
                std::cout << "\nАвто-заказ ID " << order_counter << " (Координаты: " << x << ", " << y << ")\n";

                Transport* best_transport = nullptr;
                float best_metric = std::numeric_limits<float>::max();
                float best_final_time = 0;

                for (Transport* t : fleet) {
                    if (t->isBusy()) continue;
                    if (t->canHandle(autoOrder)) {
                        float current_time = t->calculateTime(autoOrder);
                        if (current_time < best_metric) {
                            best_metric = current_time;
                            best_transport = t;
                            best_final_time = current_time;
                        }
                    }
                }

                if (best_transport != nullptr) {
                    float profit = best_transport->calculatePrice(autoOrder); 
                    coords dest = autoOrder.getDestination();
                    std::cout << "Назначено на: " << best_transport->getname() << "\n";
                    std::cout << "  Куда едет (Координаты): [" << dest.x << ", " << dest.y << "]\n";
                    std::cout << "  Время в пути: " << Transport::formatTime(best_final_time) << "\n";
                    std::cout << "  Заработок за заказ: " << std::fixed << std::setprecision(2) << profit << " руб.\n";
                    best_transport->setBusy(true);
                    
                    int travel_mins = static_cast<int>(best_final_time * 60);
                    best_transport->setTimeToFree(virtual_time + travel_mins);
                    best_transport->setPosition(autoOrder.getDestination());
                } else {
                    std::cout << "[Внимание] Нет свободного транспорта для авто-заказа!\n";
                }
                order_counter++;
            }
        }
        else if (choice == 7) {
            if (groupOrders.empty()) {
                std::cout << "\nТекущая группа заказов пуста.\n";
            } else {
                std::cout << "\n Заказы в текущей группе \n";
                for (size_t i = 0; i < groupOrders.size(); ++i) {
                    std::cout << "[" << i + 1 << "] ";
                    groupOrders[i].PrintStats();
                }

                int id_to_remove = UI::getOrderIdForDeletion();
                if (id_to_remove != 0) {
                    auto it = std::find_if(groupOrders.begin(), groupOrders.end(), 
                        [id_to_remove](const Order& ord) {
                            return ord.getID() == id_to_remove;
                        });

                    if (it != groupOrders.end()) {
                        groupOrders.erase(it); 
                        std::cout << "Заказ ID " << id_to_remove << " успешно удален из группы.\n";
                        std::cout << "Всего заказов в группе теперь: " << groupOrders.size() << "\n";
                    } else {
                        std::cout << "Заказ с ID " << id_to_remove << " не найден в группе.\n";
                    }
                }
            }
        }
        else if (choice == 1) {
            updateMarketDemand(fleet);
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            int max_time = UI::getIntInput("Макс. время доставки (минуты): ");
            
            int x = UI::getCoordInput("Координата X: ");
            int y = UI::getCoordInput("Координата Y: ");
            int cust_type = UI::getCustomerTypeChoice();

            coords dest = {x, y};
            Order newOrder(order_counter, w, v, dest, max_time, cust_type); 

            int strat_input = UI::getStrategyChoice();
            Transport* best_transport = nullptr;
            float best_metric = std::numeric_limits<float>::max();
            float best_final_time = 0;
            float best_final_price = 0;

            for (Transport* t : fleet) {
                if (t->isBusy()) continue;

                if (t->canHandle(newOrder)) {
                    float current_time = t->calculateTime(newOrder);
                    float current_price = t->calculatePrice(newOrder);

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
            }

            if (best_transport != nullptr) {
                std::cout << "\nРЕЗУЛЬТАТ (" << (strat_input == 1 ? "Быстрая стратегия" : "Самая дешевая стратегия") << "):\n";
                std::cout << "Назначено на " << best_transport->getname() 
                          << " (Время: " << Transport::formatTime(best_final_time) << ")\n";
                std::cout << "Стоимость доставки: " << std::fixed << std::setprecision(2) << best_final_price << " руб.\n";
                
                best_transport->setBusy(true);
                int travel_mins = static_cast<int>(best_final_time * 60);
                best_transport->setTimeToFree(virtual_time + travel_mins);
                best_transport->setPosition(newOrder.getDestination());
                
            } else {
                std::cout << "\nВнимание: нет свободного транспорта.\n";
            }
            order_counter++; 
        }
        else if (choice == 3) {
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            int max_time = UI::getIntInput("Макс. время доставки (минуты): ");
            
            int x = UI::getCoordInput("Координата X: ");
            int y = UI::getCoordInput("Координата Y: ");
            int cust_type = UI::getCustomerTypeChoice();

            coords dest = {x, y};
            groupOrders.push_back(Order(order_counter, w, v, dest, max_time, cust_type));
            std::cout << "Заказ добавлен в текущую группу. Всего заказов в группе: " << groupOrders.size() << "\n";
            order_counter++;
        }
        else if (choice == 4) {
            updateMarketDemand(fleet);
            int strat_input = UI::getStrategyChoice();
            performGroupDelivery(fleet, groupOrders, strat_input, virtual_time);
            groupOrders.clear();
        }
    }

    for (Transport* t : fleet) {
        delete t;
    }
    fleet.clear();

    return 0;
}