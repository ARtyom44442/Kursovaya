#include "MenuHandler.h"
#include "Simulation.h"
#include "order/order.h"
#include "logs/logger.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>

void handleSingleOrder(std::vector<Transport*>& fleet, AppState& state) {
    logger::log("[Диспетчер] Начато создание одиночного заказа вручную.");
    updateMarketDemand(fleet);
    
    float w = UI::getFloatInput("Вес (кг): ");
    float v = UI::getFloatInput("Объем (м3): ");
    int max_time = UI::getIntInput("Макс. время доставки (минуты): ");
    int x = UI::getCoordInput("Координата X: ");
    int y = UI::getCoordInput("Координата Y: ");
    int cust_type = UI::getCustomerTypeChoice();

    coords dest = {x, y};
    Order newOrder(state.order_counter, w, v, dest, max_time, cust_type);

    int strat_input = getStrategyByCustomerType(cust_type);
    
    logger::log("[Одиночный заказ] Сформирован ID " + std::to_string(state.order_counter) + 
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
        
        logger::log("[Одиночный заказ] ID " + std::to_string(state.order_counter) + 
                   " успешно назначен на " + best_transport->getname() + 
                   ". Стратегия: " + (strat_input == 1 ? "Быстрая" : "Экономичная") + 
                   ". Стоимость: " + std::to_string(best_final_price) + " руб.");
        
        best_transport->setBusy(true);
        int total_mins = static_cast<int>(total_time * 60);
        best_transport->setTimeToFree(state.virtual_time + total_mins);
        best_transport->setPosition({0, 0});
        
    } else {
        if (!has_free) {
            std::cout << "\nВнимание: нет свободного транспорта.\n";
            logger::log("[Одиночный заказ] ОТКЛОНЕН ID " + std::to_string(state.order_counter) + ". Причина: нет свободного транспорта.");
        } else if (!has_capable) {
            std::cout << "\nВнимание: нет транспорта, способного перевезти такой груз.\n";
            logger::log("[Одиночный заказ] ОТКЛОНЕН ID " + std::to_string(state.order_counter) + ". Причина: превышены габариты или нехватка заряда.");
        } else if (!has_time) {
            std::cout << "\nВнимание: ни один транспорт не успеет выполнить заказ вовремя.\n";
            logger::log("[Одиночный заказ] ОТКЛОНЕН ID " + std::to_string(state.order_counter) + ". Причина: нехватка времени на доставку.");
        } else {
            std::cout << "\nВнимание: заказ отклонен по неизвестной причине.\n";
        }
    }
    state.order_counter++;
}

void handleViewFleet(const std::vector<Transport*>& fleet) {
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

void handleAddToGroup(std::vector<Order>& groupOrders, int& order_counter) {
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

void handleGroupDelivery(std::vector<Transport*>& fleet, std::vector<Order>& groupOrders, int& virtual_time) {
    logger::log("[Диспетчер] Инициализирован процесс групповой доставки.");
    updateMarketDemand(fleet);
    performGroupDelivery(fleet, groupOrders, 0, virtual_time);
    groupOrders.clear();
}

void handleAdvanceTime(std::vector<Transport*>& fleet, int& virtual_time) {
    int skip = UI::getIntInput("Сколько минут пропустить? ");
    logger::log("[Диспетчер] Ручная перемотка времени на " + std::to_string(skip) + " минут.");
    advanceTime(skip, virtual_time, fleet);
}

void handleHalfDaySimulation(std::vector<Transport*>& fleet, AppState& state) {
    simStats = SimulationStats();
    
    state.virtual_time = 480;
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
    
    while (state.virtual_time < end_time) {
        std::cout << "\n[Ожидание] Следующий заказ через 5 секунд...\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        int time_jump = (std::rand() % 26) + 5;
        advanceTime(time_jump, state.virtual_time, fleet);
        updateMarketDemand(fleet);
        
        if (state.virtual_time >= end_time) {
            std::cout << "\n[Время] Достигнут конец рабочего дня (13:00)\n";
            break;
        }
        
        float w = (std::rand() % 150) / 10.0f + 1.0f; 
        float v = (std::rand() % 30) / 10.0f + 0.1f; 
        int max_t = (std::rand() % 120) + 40; 
        int x = (std::rand() % 201) - 100; 
        int y = (std::rand() % 201) - 100;
        int cust_type = (std::rand() % 3) + 1;

        Order autoOrder(state.order_counter, w, v, {x, y}, max_t, cust_type);
        simStats.totalOrdersGenerated++;
        
        if (cust_type == 1) simStats.vipOrders++;
        else if (cust_type == 2) simStats.expressOrders++;
        else simStats.economyOrders++;
        
        int strategy = getStrategyByCustomerType(cust_type);
        
        std::cout << "\n----------------------------------------\n";
        std::cout << "[" << formatClock(state.virtual_time) << "] ";
        std::cout << "Сгенерирован заказ ID " << state.order_counter << "\n";
        std::cout << "  Статус заказчика: " << getCustomerTypeName(cust_type) << "\n";
        std::cout << "  Вес: " << std::fixed << std::setprecision(1) << w << " кг";
        std::cout << " | Объём: " << v << " м³\n";
        std::cout << "  Координаты: (" << x << ", " << y << ")";
        std::cout << " | Макс. время: " << max_t << " мин.\n";
        
        logger::log("[Симуляция] Сгенерирован заказ ID " + std::to_string(state.order_counter) + 
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
            float return_time = best_final_time;
            float total_time = best_final_time + return_time;
            
            std::cout << "  >>> НАЗНАЧЕН: " << best_transport->getname() << "\n";
            std::cout << "      " << Transport::formatTimeWithReturn(best_final_time, return_time) << "\n";
            std::cout << "      Стоимость: " << std::fixed << std::setprecision(2) << best_final_price << " руб.\n";
            
            logger::log("[Симуляция] Заказ ID " + std::to_string(state.order_counter) + 
                       " назначен на " + best_transport->getname() + 
                       " | Время: " + std::to_string(best_final_time) + " ч" +
                       " | Стоимость: " + std::to_string(best_final_price) + " руб.");
            
            simStats.ordersDelivered++;
            simStats.totalProfit += best_final_price;
            
            best_transport->setBusy(true);
            int total_mins = static_cast<int>(total_time * 60);
            best_transport->setTimeToFree(state.virtual_time + total_mins);
            best_transport->setPosition({0, 0});
        } else {
            simStats.ordersRejected++;
            
            if (!has_free) {
                std::cout << "  >>> ОТКЛОНЁН: Нет свободного транспорта\n";
                logger::log("[Симуляция] Заказ ID " + std::to_string(state.order_counter) + " ОТКЛОНЁН: нет свободного транспорта");
            } else if (!has_capable) {
                std::cout << "  >>> ОТКЛОНЁН: Превышены габариты\n";
                logger::log("[Симуляция] Заказ ID " + std::to_string(state.order_counter) + " ОТКЛОНЁН: превышены габариты");
            } else if (!has_time) {
                std::cout << "  >>> ОТКЛОНЁН: Нехватка времени\n";
                logger::log("[Симуляция] Заказ ID " + std::to_string(state.order_counter) + " ОТКЛОНЁН: нехватка времени");
            } else {
                std::cout << "  >>> ОТКЛОНЁН: Неизвестная причина\n";
                logger::log("[Симуляция] Заказ ID " + std::to_string(state.order_counter) + " ОТКЛОНЁН: неизвестная причина");
            }
        }
        
        state.order_counter++;
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

void handleManageGroup(std::vector<Order>& groupOrders) {
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
