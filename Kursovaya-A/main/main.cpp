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
#include <thread>
#include <chrono>

struct SimulationStats {
    int totalOrdersGenerated = 0;
    int vipOrders = 0;
    int expressOrders = 0;
    int economyOrders = 0;
    int ordersDelivered = 0;
    int ordersRejected = 0;
    float totalProfit = 0.0f;
};

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
        std::cout << "Нет подходящих заказов для групповой доставки (только Express и Economy).\n";
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
        std::cout << "6. Симуляция половины рабочего дня\n";
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

            int strat_input = getStrategyByCustomerType(cust_type);
            
            logger::log("[Одиночный заказ] Сформирован ID " + std::to_string(order_counter) + 
                       " | Статус заказчика: " + getCustomerTypeName(cust_type) +
                       " | Вес: " + std::to_string(w) + " | Объем: " + std::to_string(v) + 
                       " | Координаты: " + std::to_string(x) + "," + std::to_string(y));

            Transport* best_transport = nullptr;
            float best_final_time = 0;
            float best_final_price = 0;
            
            float min_metric = std::numeric_limits<float>::max();
            
            bool has_free = false;
            bool has_capable = false;
            bool has_time = false;

            for (Transport* t : fleet) {
                if (!t->isBusy()) {
                    has_free = true;
                    if (t->canHandle(newOrder)) {
                        has_capable = true;
                        float t_time = t->calculateTime(newOrder); 
                        if (t_time <= (max_time / 60.0f)) {
                            has_time = true;
                            float metric = (strat_input == 1) ? t_time : t->calculatePrice(newOrder);
                            if (metric < min_metric) {
                                min_metric = metric;
                                best_transport = t;
                                best_final_time = t_time;
                                best_final_price = t->calculatePrice(newOrder);
                            }
                        }
                    }
                }
            }

            if (best_transport != nullptr) {
                float return_time = best_final_time;
                float total_time = best_final_time + return_time;
                
                std::cout << "\nРЕЗУЛЬТАТ (" << (strat_input == 1 ? "Быстрая стратегия" : "Экономичная стратегия") << "):\n";
                std::cout << "Назначено на " << best_transport->getname() 
                          << " (" << Transport::formatTimeWithReturn(best_final_time, return_time) << ")\n";
                std::cout << "Стоимость доставки: " << std::fixed << std::setprecision(2) << best_final_price << " руб.\n";
                
                logger::log("[Одиночный заказ] ID " + std::to_string(order_counter) + 
                           " успешно назначен на " + best_transport->getname() + 
                           ". Стратегия: " + (strat_input == 1 ? "Быстрая" : "Экономичная") + 
                           ". Стоимость: " + std::to_string(best_final_price) + " руб.");
                
                best_transport->setBusy(true);
                int total_mins = static_cast<int>(total_time * 60);
                best_transport->setTimeToFree(virtual_time + total_mins);
                best_transport->setPosition({0, 0});
                
            } else {
                if (!has_free) {
                    std::cout << "\nВнимание: нет свободного транспорта.\n";
                    logger::log("[Одиночный заказ] ОТКЛОНЕН ID " + std::to_string(order_counter) + ". Причина: нет свободного транспорта.");
                } else if (!has_capable) {
                    std::cout << "\nВнимание: нет транспорта, способного перевезти такой груз.\n";
                    logger::log("[Одиночный заказ] ОТКЛОНЕН ID " + std::to_string(order_counter) + ". Причина: превышены габариты или нехватка заряда.");
                } else if (!has_time) {
                    std::cout << "\nВнимание: ни один транспорт не успеет выполнить заказ вовремя.\n";
                    logger::log("[Одиночный заказ] ОТКЛОНЕН ID " + std::to_string(order_counter) + ". Причина: нехватка времени на доставку.");
                } else {
                    std::cout << "\nВнимание: заказ отклонен по неизвестной причине.\n";
                }
            }
            order_counter++;
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
        else if (choice == 3) {
            float w = UI::getFloatInput("Вес (кг): ");
            float v = UI::getFloatInput("Объем (м3): ");
            int max_time = UI::getIntInput("Макс. время доставки (минуты): ");
            int x = UI::getCoordInput("Координата X: ");
            int y = UI::getCoordInput("Координата Y: ");
            
            int menu_choice = 0;
            int cust_type = 0;
            while (true) {
                std::cout << "\nКатегория заказчика для групповой доставки:\n"
                          << "1. Экспресс-класс\n"
                          << "2. Эконом-класс\n"
                          << "Выбор: ";
                if (std::cin >> menu_choice && (menu_choice == 1 || menu_choice == 2)) {
                    std::cin.ignore(10000, '\n');
                    cust_type = (menu_choice == 1) ? 2 : 3;
                    break;
                }
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "  [Ошибка] Выберите 1 или 2.\n";
            }

            coords dest = {x, y};
            groupOrders.push_back(Order(order_counter, w, v, dest, max_time, cust_type));
            std::cout << "Заказ добавлен в текущую группу. Всего заказов в группе: " << groupOrders.size() << "\n";
            
            logger::log("[Группа] Заказ ID " + std::to_string(order_counter) + " вручную добавлен в пул групповой доставки. Тип: " + getCustomerTypeName(cust_type));
            order_counter++;
        }
        else if (choice == 4) {
            logger::log("[Диспетчер] Инициализирован процесс групповой доставки.");
            updateMarketDemand(fleet);
            performGroupDelivery(fleet, groupOrders, 0, virtual_time);
            groupOrders.clear();
        }
        else if (choice == 5) {
            int skip = UI::getIntInput("Сколько минут пропустить? ");
            logger::log("[Диспетчер] Ручная перемотка времени на " + std::to_string(skip) + " минут.");
            advanceTime(skip, virtual_time, fleet);
        }
        else if (choice == 6) {
            simStats = SimulationStats();
            
            virtual_time = 480;
            int end_time = 780;
            
            logger::log("============================================");
            logger::log("[СИМУЛЯЦИЯ] Запуск симуляции полдня (08:00 - 13:00)");
            logger::log("[СИМУЛЯЦИЯ] Заказы генерируются каждые 5 реальных секунд");
            logger::log("============================================");
            std::cout << "\n============================================\n";
            std::cout << "    СИМУЛЯЦИЯ ПОЛОВИНЫ РАБОЧЕГО ДНЯ\n";
            std::cout << "    Время: 08:00 - 13:00\n";
            std::cout << "    Каждый заказ через 5 реальных секунд\n";
            std::cout << "============================================\n\n";
            
            while (virtual_time < end_time) {
                std::cout << "\n[Ожидание] Следующий заказ через 5 секунд...\n";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                
                int time_jump = (std::rand() % 26) + 5;
                advanceTime(time_jump, virtual_time, fleet);
                updateMarketDemand(fleet);
                
                if (virtual_time >= end_time) {
                    std::cout << "\n[Время] Достигнут конец рабочего дня (13:00)\n";
                    break;
                }
                
                float w = (std::rand() % 150) / 10.0f + 1.0f; 
                float v = (std::rand() % 30) / 10.0f + 0.1f; 
                int max_t = (std::rand() % 120) + 40; 
                int x = (std::rand() % 201) - 100; 
                int y = (std::rand() % 201) - 100;
                int cust_type = (std::rand() % 3) + 1;

                Order autoOrder(order_counter, w, v, {x, y}, max_t, cust_type);
                simStats.totalOrdersGenerated++;
                
                if (cust_type == 1) simStats.vipOrders++;
                else if (cust_type == 2) simStats.expressOrders++;
                else simStats.economyOrders++;
                
                int strategy = getStrategyByCustomerType(cust_type);
                
                std::cout << "\n----------------------------------------\n";
                std::cout << "[" << formatClock(virtual_time) << "] ";
                std::cout << "Сгенерирован заказ ID " << order_counter << "\n";
                std::cout << "  Статус заказчика: " << getCustomerTypeName(cust_type) << "\n";
                std::cout << "  Вес: " << std::fixed << std::setprecision(1) << w << " кг";
                std::cout << " | Объём: " << v << " м³\n";
                std::cout << "  Координаты: (" << x << ", " << y << ")";
                std::cout << " | Макс. время: " << max_t << " мин.\n";
                
                logger::log("[Симуляция] Сгенерирован заказ ID " + std::to_string(order_counter) + 
                           " | Статус заказчика: " + getCustomerTypeName(cust_type) +
                           " | Координаты: " + std::to_string(x) + "," + std::to_string(y));

                Transport* best_transport = nullptr;
                float best_metric = std::numeric_limits<float>::max();
                float best_final_time = 0;
                float best_final_price = 0;
                
                bool has_free = false;
                bool has_capable = false;
                bool has_time = false;

                for (Transport* t : fleet) {
                    if (!t->isBusy()) {
                        has_free = true;
                        if (t->canHandle(autoOrder)) {
                            has_capable = true;
                            float current_time = t->calculateTime(autoOrder);
                            float current_price = t->calculatePrice(autoOrder);
                            if (current_time <= (max_t / 60.0f)) {
                                has_time = true;
                                float metric = (strategy == 1) ? current_time : current_price;
                                if (metric < best_metric) {
                                    best_metric = metric;
                                    best_transport = t;
                                    best_final_time = current_time;
                                    best_final_price = current_price;
                                }
                            }
                        }
                    }
                }

                if (best_transport != nullptr) {
                    coords dest = autoOrder.getDestination();
                    
                    float return_time = best_final_time;
                    float total_time = best_final_time + return_time;
                    
                    std::cout << "  >>> НАЗНАЧЕН: " << best_transport->getname() << "\n";
                    std::cout << "      " << Transport::formatTimeWithReturn(best_final_time, return_time) << "\n";
                    std::cout << "      Стоимость: " << std::fixed << std::setprecision(2) << best_final_price << " руб.\n";
                    
                    logger::log("[Симуляция] Заказ ID " + std::to_string(order_counter) + 
                               " назначен на " + best_transport->getname() + 
                               " | Время: " + std::to_string(best_final_time) + " ч" +
                               " | Стоимость: " + std::to_string(best_final_price) + " руб.");
                    
                    simStats.ordersDelivered++;
                    simStats.totalProfit += best_final_price;
                    
                    best_transport->setBusy(true);
                    int total_mins = static_cast<int>(total_time * 60);
                    best_transport->setTimeToFree(virtual_time + total_mins);
                    best_transport->setPosition({0, 0});
                } else {
                    simStats.ordersRejected++;
                    
                    if (!has_free) {
                        std::cout << "  >>> ОТКЛОНЁН: Нет свободного транспорта\n";
                        logger::log("[Симуляция] Заказ ID " + std::to_string(order_counter) + " ОТКЛОНЁН: нет свободного транспорта");
                    } else if (!has_capable) {
                        std::cout << "  >>> ОТКЛОНЁН: Превышены габариты\n";
                        logger::log("[Симуляция] Заказ ID " + std::to_string(order_counter) + " ОТКЛОНЁН: превышены габариты");
                    } else if (!has_time) {
                        std::cout << "  >>> ОТКЛОНЁН: Нехватка времени\n";
                        logger::log("[Симуляция] Заказ ID " + std::to_string(order_counter) + " ОТКЛОНЁН: нехватка времени");
                    } else {
                        std::cout << "  >>> ОТКЛОНЁН: Неизвестная причина\n";
                        logger::log("[Симуляция] Заказ ID " + std::to_string(order_counter) + " ОТКЛОНЁН: неизвестная причина");
                    }
                }
                
                order_counter++;
            }
            
            std::cout << "\n============================================\n";
            std::cout << "         ИТОГОВЫЙ ОТЧЁТ СИМУЛЯЦИИ\n";
            std::cout << "============================================\n";
            std::cout << "Период симуляции: 08:00 - 13:00 (5 часов)\n\n";
            
            std::cout << "--- ЗАКАЗЫ ---\n";
            std::cout << "Всего сгенерировано заказов: " << simStats.totalOrdersGenerated << "\n";
            std::cout << "  - VIP:      " << simStats.vipOrders << "\n";
            std::cout << "  - Экспресс-класс  " << simStats.expressOrders << "\n";
            std::cout << "  - Эконом-класс:  " << simStats.economyOrders << "\n\n";
            
            std::cout << "--- ДОСТАВКА ---\n";
            std::cout << "Успешно доставлено: " << simStats.ordersDelivered << "\n";
            std::cout << "Отклонено:          " << simStats.ordersRejected << "\n\n";
            
            std::cout << "--- ФИНАНСЫ ---\n";
            std::cout << "Общая прибыль: " << std::fixed << std::setprecision(2) << simStats.totalProfit << " руб.\n";
            std::cout << "============================================\n";
            
            logger::log("============================================");
            logger::log("[ИТОГИ СИМУЛЯЦИИ] Период: 08:00 - 13:00");
            logger::log("[ИТОГИ] Всего заказов: " + std::to_string(simStats.totalOrdersGenerated) + 
                       " | VIP: " + std::to_string(simStats.vipOrders) + 
                       " | Экспресс-класс: " + std::to_string(simStats.expressOrders) + 
                       " | Эконом-класс: " + std::to_string(simStats.economyOrders));
            logger::log("[ИТОГИ] Доставлено: " + std::to_string(simStats.ordersDelivered) + 
                       " | Отклонено: " + std::to_string(simStats.ordersRejected));
            logger::log("[ИТОГИ] Общая прибыль: " + std::to_string(simStats.totalProfit) + " руб.");
            logger::log("============================================");
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
    }

    for (Transport* t : fleet) {
        delete t;
    }
    fleet.clear();

    return 0;
}
