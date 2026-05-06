#include <iostream>
#include <vector>
#include "order.h"
#include "datareader.h"
#include "Transport.h"

int main() {

    DataReader reader;
    std::vector<Transport*> fleet = reader.loadTransports("data/transports.json");

    if (fleet.empty()) {
        std::cerr << "Критическая ошибка: Автопарк пуст! Проверьте файл data/transports.json\n";
        return 1;
    }

    std::cout << "Успешно загружено единиц транспорта: " << fleet.size() << "\n\n";

    int choice = -1;
    int order_counter = 1;

    while (true) {
        std::cout << "ГЛАВНОЕ МЕНЮ\n";
        std::cout << "1. Создать новый заказ и найти транспорт\n";
        std::cout << "2. Посмотреть весь доступный транспорт\n";
        std::cout << "0. Выход из программы\n";
        std::cout << "Ваш выбор: ";
        
        std::cin >> choice;

        if (choice == 0) {
            std::cout << "Завершение работы диспетчера...\n";
            break;
        }
        else if (choice == 2) {
            std::cout << "\nДОСТУПНЫЙ АВТОПАРК\n";
            for (Transport* t : fleet) {
                std::cout << "- " << t->getname() << " (Скорость: " << t->getspeed() << ")\n";
            }
            std::cout << "\n";
        }
        else if (choice == 1) {
            // 3. Создание заказа (Интерактив)
            float w, v;
            int x, y;
            
            std::cout << "\nОФОРМЛЕНИЕ ЗАКАЗА #" << order_counter << "\n";
            std::cout << "Введите вес (кг): ";
            std::cin >> w;
            std::cout << "Введите объем (м3): ";
            std::cin >> v;
            std::cout << "Введите координату доставки X: ";
            std::cin >> x;
            std::cout << "Введите координату доставки Y: ";
            std::cin >> y;

            coords dest = {x, y};
            Order newOrder(order_counter, w, v, dest, 120); 

            std::cout << "\nРАСПРЕДЕЛЕНИЕ\n";
            
            Transport* best_transport = nullptr;
            float best_time = 999999.0;

            for (Transport* t : fleet) {
                if (t->canHandle(newOrder)) {
                    float current_time = t->calculateTime(newOrder);
                    std::cout << "[ОК] " << t->getname() << " справится за " << current_time << " ед. времени.\n";
                    
                    if (current_time < best_time) {
                        best_time = current_time;
                        best_transport = t;
                    }
                } else {
                    std::cout << "[ОТКАЗ] " << t->getname() << " не подходит (ограничения).\n";
                }
            }

            if (best_transport != nullptr) {
                std::cout << "\nРЕЗУЛЬТАТ: Заказ назначен на " << best_transport->getname() 
                          << " (Ожидаемое время: " << best_time << ")\n\n";
            } else {
                std::cout << "\nВНИМАНИЕ: Нет доступного транспорта для такого заказа!\n\n";
            }
            
            order_counter++;
        }
        else {
            std::cout << "Неверный ввод. Попробуйте снова.\n\n";
        }
    }

    for (Transport* t : fleet) {
        delete t;
    }
    fleet.clear();

    return 0;
}