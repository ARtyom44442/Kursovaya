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
        if (t->isBusy()) continue;

        RouteInfo route;
        if (t->buildRoute(groupOrders, route)) {
            validRoutes.push_back(route);
        }
    }

    if (validRoutes.empty()) {
        std::cout << "\nВНИМАНИЕ: Нет доступного (свободного) транспорта для этой группы заказов.\n";
        logger::log("Групповая доставка: подходящий транспорт не найден или весь занят");
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

    std::cout << "\nРЕЗУЛЬТАТ ГРУППОВОЙ ДОСТАВКИ:\n";
    std::cout << "Назначено на " << best_route.transport->getname() << "\n";
    std::cout << "Общее время: " << Transport::formatTime(best_route.totalTime) << "\n";
    std::cout << "Общая стоимость: " << std::fixed << std::setprecision(2) << best_route.totalPrice << " руб.\n";
    
    best_route.transport->setBusy(true);
    if (!best_route.orderIndices.empty()) {
        best_route.transport->setPosition(groupOrders[best_route.orderIndices.back()].getDestination());
    }

    logger::log("Групповая доставка успешно назначена на транспорт: " + best_route.transport->getname());
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
        std::cout << "\n[ДИНАМИЧЕСКИЙ ТАРИФ] Высокий спрос! Занято " 
                  << std::fixed << std::setprecision(0) << (busyPercentage * 100) 
                  << "% транспорта. Цены временно повышены на 40%.\n";
        logger::log("Surge Pricing активирован: коэффициент спроса x1.40 (Занято " + std::to_string(busyCount) + " машин)");
    } 
    else if (busyPercentage < 0.15f) {
        Transport::setDemandFactor(0.85f); 
        std::cout << "\n[ДИНАМИЧЕСКИЙ ТАРИФ] Низкий спрос. Занято всего " 
                  << std::fixed << std::setprecision(0) << (busyPercentage * 100) 
                  << "% транспорта. Действует скидка 15%!\n";
        logger::log("Скидка при низком спросе: коэффициент спроса x0.85");
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
        std::cerr << "Ошибка: Автопарк пуст\n";
        logger::log("Критическая ошибка: Автопарк пуст, завершение работы");
        return 1;
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    for (Transport* t : fleet) {
        int rx = (std::rand() % 201) - 100;
        int ry = (std::rand() % 201) - 100;
        t->setPosition({rx, ry});
    }

    logger::log("Автопарк загружен. Транспорту заданы случайные координаты. Единиц: " + std::to_string(fleet.size()));

    std::vector<Order> groupOrders;
    int choice = -1;
    int order_counter = 1;

    while (true) {
        std::cout << "\nГЛАВНОЕ МЕНЮ\n";
        std::cout << "1. Создать одиночный заказ\n";
        std::cout << "2. Посмотреть доступный транспорт\n";
        std::cout << "3. Добавить заказ в группу\n";
        std::cout << "4. Выполнить групповую доставку\n";
        std::cout << "5. Завершить текущие рейсы (Освободить транспорт)\n";
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
            logger::log("Запрошен просмотр автопарка");
            std::cout << "\nСОСТОЯНИЕ АВТОПАРКА\n";
            for (Transport* t : fleet) {
                std::string status = t->isBusy() ? "[В РЕЙСЕ]" : "[СВОБОДЕН]";
                coords pos = t->getCurrentPos();
                std::cout << status << " " << t->getname() 
                          << " | Координаты: (" << pos.x << ", " << pos.y << ")\n";
            }
        }
        else if (choice == 1) {
            updateMarketDemand(fleet);
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            int max_time = UI::getIntInput("Макс. время доставки (минуты): ");
            int x = UI::getIntInput("Координата X: ");
            int y = UI::getIntInput("Координата Y: ");

            coords dest = {x, y};
            Order newOrder(order_counter, w, v, dest, max_time);
            logger::log("Создан одиночный заказ ID: " + std::to_string(order_counter));

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
                best_transport->setPosition(newOrder.getDestination());
                
                logger::log("Заказ ID " + std::to_string(order_counter) + " распределен на " + best_transport->getname());
            } else {
                std::cout << "\nВНИМАНИЕ: Нет доступного (или свободного) транспорта для этого заказа.\n";
                logger::log("Предупреждение: Не удалось найти свободный транспорт для заказа ID " + std::to_string(order_counter));
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
            logger::log("Заказ ID " + std::to_string(order_counter) + " добавлен в пул");
            order_counter++;
        }
        else if (choice == 4) {
            updateMarketDemand(fleet);
            int strat_input = UI::getStrategyChoice();
            performGroupDelivery(fleet, groupOrders, strat_input);
            groupOrders.clear();
        }
        else if (choice == 5) {
            int freed_count = 0;
            for (Transport* t : fleet) {
                if (t->isBusy()) {
                    t->setBusy(false);
                    freed_count++;
                }
            }
            std::cout << "\nОсвобождено машин: " << freed_count << "\n";
            logger::log("Диспетчер вручную завершил все рейсы. Освобождено единиц: " + std::to_string(freed_count));
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