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
#include "logger.h"
#include <algorithm>   
#include <climits>     

void performGroupDelivery(const std::vector<Transport*>& fleet, std::vector<Order>& groupOrders, int strategy) {
    if (groupOrders.empty()) {
        std::cout << "Нет заказов в группе. Сначала добавьте заказы (пункт 3).\n";
        logger::log("Попытка групповой доставки с пустым списком заказов");
        return;
    }
    if (!Order::areOrdersClose(groupOrders, 200)) {
        std::cout << "Заказы расположены слишком далеко друг от друга (более 200 км). Группировка невозможна.\n";
        logger::log("Отказ группировки: заказы находятся слишком далеко друг от друга");
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
        std::cout << "\nВНИМАНИЕ: Нет доступного транспорта для этой группы заказов.\n";
        logger::log("Групповая доставка: подходящий транспорт не найден");
        return;
    }

    RouteInfo best_route = validRoutes[0];
    for (const auto& r : validRoutes) {
        if (strategy == 1) {
            if (r.totalTime < best_route.totalTime) {
                best_route = r;
            }
        } else if (strategy == 2) {
            if (r.totalPrice < best_route.totalPrice) {
                best_route = r;
            }
        }
    }

    std::cout << "\nРЕЗУЛЬТАТ ГРУППОВОЙ ДОСТАВКИ:\n";
    std::cout << "Назначено на " << best_route.transport->getname() << "\n";
    std::cout << "Общее время: " << Transport::formatTime(best_route.totalTime) << "\n";
    std::cout << "Общая стоимость: " << std::fixed << std::setprecision(2) << best_route.totalPrice << " руб.\n";
    
    logger::log("Групповая доставка успешно назначена на транспорт: " + best_route.transport->getname());
}

int main() {
    std::setlocale(LC_ALL, "Russian");
    logger::init("logs/logs.txt");

    DataReader reader;
    std::vector<Transport*> fleet = reader.loadTransports("data/transports.json");

    if (fleet.empty()) {
        std::cerr << "Ошибка: Автопарк пуст\n";
        logger::log("Критическая ошибка: Автопарк пуст, завершение работы");
        return 1;
    }
    logger::log("Автопарк успешно загружен. Доступно единиц транспорта: " + std::to_string(fleet.size()));

    std::vector<Order> groupOrders;
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

        std::cin >> choice;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Неверный выбор меню\n";
            logger::log("Предупреждение: Некорректный ввод пункта меню");
            continue;
        }

        if (choice == 0) {
            logger::log("Система диспетчеризации успешно завершила работу");
            break;
        }
        else if (choice == 2) {
            logger::log("Запрошен просмотр доступного автопарка");
            std::cout << "\nДОСТУПНЫЙ АВТОПАРК\n";
            for (Transport* t : fleet) {
                std::cout << t->getname() << " (Скорость: " << t->getspeed() << ")\n";
            }
        }
        else if (choice == 1) {
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            int max_time = UI::getIntInput("Макс. время доставки (минуты): ");
            int x = UI::getIntInput("Координата X: ");
            int y = UI::getIntInput("Координата Y: ");

            coords dest = {x, y};
            Order newOrder(order_counter, w, v, dest, max_time);
            logger::log("Создан одиночный заказ ID: " + std::to_string(order_counter) + " (Вес: " + std::to_string(w) + ", Объем: " + std::to_string(v) + ")");

            int strat_input = UI::getStrategyChoice();
            logger::log("Для заказа ID " + std::to_string(order_counter) + " выбрана стратегия поиска: " + std::to_string(strat_input));

            Transport* best_transport = nullptr;
            float best_metric = std::numeric_limits<float>::max();
            float best_final_time = 0;
            float best_final_price = 0;

            for (Transport* t : fleet) {
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
                logger::log("Заказ ID " + std::to_string(order_counter) + " успешно распределен на " + best_transport->getname() + " за " + std::to_string(best_final_price) + " руб.");
            } else {
                std::cout << "\nВНИМАНИЕ: Нет доступного транспорта для этого заказа даже с учетом стратегии.\n";
                logger::log("Предупреждение: Не удалось найти подходящий транспорт для одиночного заказа ID " + std::to_string(order_counter));
            }
            order_counter++; 
        }
        else if (choice == 3) {
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            int max_time = UI::getIntInput("Макс. время доставки (минуты): ");
            int x = UI::getIntInput("Координата X: ");
            int y = UI::getIntInput("Координата Y: ");

            coords dest = {x, y};
            groupOrders.push_back(Order(order_counter, w, v, dest, max_time));
            std::cout << "Заказ добавлен в текущую группу. Всего заказов в группе: " << groupOrders.size() << "\n";
            logger::log("Заказ ID " + std::to_string(order_counter) + " добавлен в пул для групповой доставки");
            order_counter++;
        }
        else if (choice == 4) {
            int strat_input = UI::getStrategyChoice();
            logger::log("Запрошено выполнение групповой доставки. Всего заказов: " + std::to_string(groupOrders.size()) + ", стратегия: " + std::to_string(strat_input));
            performGroupDelivery(fleet, groupOrders, strat_input);
            groupOrders.clear();
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
//