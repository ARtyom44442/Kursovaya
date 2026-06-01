#include <iostream>
#include <vector>
#include <clocale>
#include <cstdlib>
#include <ctime>
#include "order/order.h"
#include "data/datareader.h"
#include "transport/Transport.h"
#include "Simulation.h"
#include "MenuHandler.h"
#include "logs/logger.h"

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
    
    AppState state;
    int choice = -1;

    while (true) {
        std::cout << "\nГлавное меню | Время в симуляции: " << formatClock(state.virtual_time) << "\n";
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

        switch (choice) {
            case 0:
                logger::log("[Система] Завершение работы программы.");
                break;
            case 1:
                handleSingleOrder(fleet, state);
                break;
            case 2:
                handleViewFleet(fleet);
                break;
            case 3:
                handleAddToGroup(state.groupOrders, state.order_counter);
                break;
            case 4:
                handleGroupDelivery(fleet, state.groupOrders, state.virtual_time);
                break;
            case 5:
                handleAdvanceTime(fleet, state.virtual_time);
                break;
            case 6:
                handleHalfDaySimulation(fleet, state);
                break;
            case 7:
                handleManageGroup(state.groupOrders);
                break;
            default:
                std::cout << "Неверный ввод\n";
                break;
        }
        
        if (choice == 0) break;
    }

    for (Transport* t : fleet) {
        delete t;
    }
    fleet.clear();

    return 0;
}
