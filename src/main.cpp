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
    if (!Order::areOrdersClose(groupOrders, 200)) {
        std::cout << "Заказы слишком далеко друг от друга.\n";
        logger::log("[Группа] Ошибка доставки: заказы расположены слишком далеко.");
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
        logger::log("[Группа] Ошибка: нет доступного транспорта для выполнения групповой доставки.");
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
    
    std::string order_list = "";
    std::cout << "Порядок доставки (ID заказов): ";
    for (size_t i = 0; i < best_route.orderIndices.size(); ++i) {
        std::cout << groupOrders[best_route.orderIndices[i]].getID();
        order_list += std::to_string(groupOrders[best_route.orderIndices[i]].getID()) + " ";
        if (i < best_route.orderIndices.size() - 1) std::cout << " -> ";
    }
    std::cout << "\n";

    best_route.transport->setBusy(true);
    int delivery_time_mins = static_cast<int>(best_route.totalTime * 60);
    best_route.transport->setTimeToFree(virtual_time + delivery_time_mins);

    if (!best_route.orderIndices.empty()) {
        best_route.transport->setPosition(groupOrders[best_route.orderIndices.back()].getDestination());
    }
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

int main() {
    std::setlocale(LC_ALL, "ru_RU.UTF-8");
    logger::init("logs/logs.txt");

    DataReader reader;
    std::vector<Transport*> fleet = reader.loadTransports("data/transports.json");

    if (fleet.empty()) {
        std::cerr << "Ошибка: автопарк пуст\n";
        logger::log("[Ошибка] Критический сбой: Автопарк пуст или файл transports.json не найден.");
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

        if (choice == 0) {
            logger::log("[Система] Завершение работы программы.");
            break;
        }
        else if (choice == 2) {
            logger::log("[Диспетчер] Запрошен просмотр состояния автопарка.");
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
            logger::log("[Диспетчер] Ручная перемотка времени на " + std::to_string(skip) + " минут.");
            advanceTime(skip, virtual_time, fleet);
        }
        else if (choice == 6) {
            int sim_count = UI::getIntInput("Сколько заказов сгенерировать? ");
            logger::log("[Авто-симуляция] Запуск генератора на " + std::to_string(sim_count) + " заказов.");
            
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
                logger::log("[Авто-заказ] Сгенерирован ID " + std::to_string(order_counter) + " | Вес: " + std::to_string(w) + " | Объем: " + std::to_string(v) + " | Координаты: " + std::to_string(x) + "," + std::to_string(y) + " | Приоритет: " + std::to_string(cust_type));

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
                    
                    logger::log("[Авто-заказ] ID " + std::to_string(order_counter) + " успешно назначен на " + best_transport->getname() + ". Прибыль: " + std::to_string(profit) + " руб.");
                    
                    best_transport->setBusy(true);
                    int travel_mins = static_cast<int>(best_final_time * 60);
                    best_transport->setTimeToFree(virtual_time + travel_mins);
                    best_transport->setPosition(autoOrder.getDestination());
                } else {
                    std::cout << "[Внимание] Нет свободного транспорта для авто-заказа!\n";
                    logger::log("[Авто-заказ] ОТКЛОНЕН ID " + std::to_string(order_counter) + ". Причина: нет свободного или подходящего по характеристикам транспорта.");
                }
                order_counter++;
            }
            logger::log("[Авто-симуляция] Цикл генерации завершен.");
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
                        logger::log("[Группа] Заказ ID " + std::to_string(id_to_remove) + " удален из текущей группы.");
                    } else {
                        std::cout << "Заказ с ID " << id_to_remove << " не найден в группе.\n";
                    }
                }
            }
        }
        else if (choice == 1) {
            logger::log("[Диспетчер] Начато создание одиночного заказа вручную.");
            updateMarketDemand(fleet);
            
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            int max_time = UI::getIntInput("Макс. время доставки (минуты): ");
            int x = UI::getCoordInput("Координата X: ");
            int y = UI::getCoordInput("Координата Y: ");
            int cust_type = UI::getCustomerTypeChoice();

            coords dest = {x, y};
            Order newOrder(order_counter, w, v, dest, max_time, cust_type); 

            logger::log("[Одиночный заказ] Сформирован ID " + std::to_string(order_counter) + " | Вес: " + std::to_string(w) + " | Объем: " + std::to_string(v) + " | Координаты: " + std::to_string(x) + "," + std::to_string(y));

            int strat_input = UI::getStrategyChoice();
            Transport* best_transport = nullptr;
            float best_final_time = 0;
            float best_final_price = 0;
            
            float min_metric = 9999999999999.0f;

            for (Transport* t : fleet) {
                if (!t->isBusy() && t->canHandle(newOrder)) {
                    float t_time = t->calculateTime(newOrder); 
                    
                    if (t_time <= (max_time / 60.0f)) {
                        if (strat_input == 1) { 
                            if (t_time < min_metric) {
                                min_metric = t_time;
                                best_transport = t;
                                best_final_time = t_time;
                                best_final_price = t->calculatePrice(newOrder); 
                            }
                        } 
                        else if (strat_input == 2) { 
                            float t_price = t->calculatePrice(newOrder);
                            if (t_price < min_metric) {
                                min_metric = t_price;
                                best_transport = t;
                                best_final_time = t_time;
                                best_final_price = t_price; 
                            }
                        }
                    }
                }
            }

            if (best_transport != nullptr) {
                std::cout << "\nРЕЗУЛЬТАТ (" << (strat_input == 1 ? "Быстрая стратегия" : "Самая дешевая стратегия") << "):\n";
                std::cout << "Назначено на " << best_transport->getname() 
                          << " (Время: " << Transport::formatTime(best_final_time) << ")\n";
                std::cout << "Стоимость доставки: " << std::fixed << std::setprecision(2) << best_final_price << " руб.\n";
                
                logger::log("[Одиночный заказ] ID " + std::to_string(order_counter) + " успешно назначен на " + best_transport->getname() + ". Стратегия: " + std::to_string(strat_input) + ". Стоимость: " + std::to_string(best_final_price) + " руб.");
                
                best_transport->setBusy(true);
                int travel_mins = static_cast<int>(best_final_time * 60);
                best_transport->setTimeToFree(virtual_time + travel_mins);
                best_transport->setPosition(newOrder.getDestination());
                
            } else {
                std::cout << "\nВнимание: нет свободного транспорта.\n";
                logger::log("[Одиночный заказ] ОТКЛОНЕН ID " + std::to_string(order_counter) + ". Причина: нет свободного или подходящего транспорта.");
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
            
            logger::log("[Группа] Заказ ID " + std::to_string(order_counter) + " вручную добавлен в пул групповой доставки.");
            order_counter++;
        }
        else if (choice == 4) {
            logger::log("[Диспетчер] Инициализирован процесс групповой доставки.");
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