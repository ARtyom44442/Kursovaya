#include <iostream>
#include <vector>
#include <limits>
#include <clocale>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "order.h"
#include "datareader.h"
#include "Transport.h"
#include <algorithm>   
#include <climits>     


void performGroupDelivery(const std::vector<Transport*>& fleet, std::vector<Order>& groupOrders, int strategy) {
    if (groupOrders.empty()) {
        std::cout << "Нет заказов в группе. Сначала добавьте заказы (пункт 3).\n";
        return;
    }
    if (!Order::areOrdersClose(groupOrders, 200)) {
        std::cout << "Заказы расположены слишком далеко друг от друга (более 200 км). Группировка невозможна.\n";
        return;
    }

    if (strategy == 2) {
        std::cout << "\n[Информация] Экономичная стратегия. Лимиты времени всех заказов в группе увеличены в 2 раза.\n";
        for (auto& ord : groupOrders) {
            ord = Order(ord.getID(), ord.getWeight(), ord.getVol(), ord.getDestination(), ord.getMaxTime() * 2);
        }
    }

    std::vector<RouteInfo> validRoutes;
    for (Transport* t : fleet) {
        RouteInfo route;
        if (t->buildRoute(groupOrders, route)) {
            validRoutes.push_back(route);
        }
    }

    if (validRoutes.empty()) {
        std::cout << "Не удалось построить ни одного допустимого маршрута для этой группы.\n";
        return;
    }

    if (strategy == 1) {
        std::sort(validRoutes.begin(), validRoutes.end(), [](const RouteInfo& a, const RouteInfo& b) {
            return a.totalTime < b.totalTime;
        });
    } else {
        std::sort(validRoutes.begin(), validRoutes.end(), [](const RouteInfo& a, const RouteInfo& b) {
            return a.totalPrice < b.totalPrice;
        });
    }

    const auto& best = validRoutes.front();
    std::cout << "ОПТИМАЛЬНЫЙ МАРШРУТ ДЛЯ ГРУППЫ ЗАКАЗОВ:\n";
    std::cout << "Транспорт: " << best.transport->getname() << "\n";
    std::cout << "Общее время доставки: " << Transport::formatTime(best.totalTime) << "\n";
    std::cout << "Общая стоимость: " << std::fixed << std::setprecision(2) << best.totalPrice << " руб.\n";
    std::cout << "Порядок доставки заказов (ID):\n";

    for (size_t i = 0; i < best.orderIndices.size(); ++i) {
        int idx = best.orderIndices[i];
        std::cout << "  " << i + 1 << ". Заказ ID: " << groupOrders[idx].getID() 
                  << " (Время прибытия: " << Transport::formatTime(best.arrivalTimes[i]) << ")\n";
    }
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
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            float max_time = UI::getFloatInput("Макс. время доставки (часы): ");
            int x = UI::getIntInput("Координата X: ");
            int y = UI::getIntInput("Координата Y: ");

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
            int strategy = UI::getStrategyChoice();
            performGroupDelivery(fleet, groupOrders, strategy);
            groupOrders.clear();
        }
       else if (choice == 1) {
            std::cout << "\n ОФОРМЛЕНИЕ ОДИНОЧНОГО ЗАКАЗА \n";
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            float max_time = UI::getFloatInput("Макс. время доставки (часы): ");
            int x = UI::getIntInput("Координата X: ");
            int y = UI::getIntInput("Координата Y: ");

            coords dest = { x, y };
            
            int strat_input = UI::getStrategyChoice();

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

                if(!t->canHandle(newOrder)){
                    std::cout << t->getname() << " не подходит (ограничения веса/объема)\n";
                    continue;
                }
                
                float current_time = t->calculateTime(newOrder);
                if (current_time > max_time) {
                    std::cout << t->getname() << " не успеет (нужно: "
                              << Transport::formatTime(current_time) << ", жесткий лимит: " << max_time << " ч)\n";
                    continue;
                }
                
                float current_price = t->calculatePrice(newOrder);

                std::cout << t->getname() << " справится за " << Transport::formatTime(current_time)
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
                          << " (Время: " << Transport::formatTime(best_final_time) << ")\n";
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
