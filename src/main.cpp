#include <iostream>
#include <vector>
#include <limits>
#include "order.h"
#include "datareader.h"
#include "Transport.h"

bool isValidInput(float w, float v, int x, int y, float t) {
    if (w <= 0 || w > 10000) return false;
    if (v <= 0 || v > 10000) return false;
    if (x < 0 || x > 10000) return false;
    if (y < 0 || y > 10000) return false;
    if (t <= 0 || t > 10000) return false;
    return true;
}

int main() {

    DataReader reader;
    std::vector<Transport*> fleet = reader.loadTransports("data/transports.json");

    if (fleet.empty()) {
        std::cerr << "Ошибка: Автопарк пуст\n";
        return 1;
    }

    int choice = -1;
    int order_counter = 1;

    while (true) {
        std::cout << "\nГЛАВНОЕ МЕНЮ\n";
        std::cout << "1. Создать новый заказ\n";
        std::cout << "2. Посмотреть доступный транспорт\n";
        std::cout << "0. Выход\n";
        std::cout << "Выбор: ";
        
        std::cin >> choice;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Неверный ввод\n";
            continue;
        }

        if (choice == 0) {
            break;
        }
        else if (choice == 2) {
            std::cout << "\nДОСТУПНЫЙ АВТОПАРК\n";
            for (Transport* t : fleet) {
                std::cout << t->getname() << " (Скорость: " << t->getspeed() << ")\n";
            }
        }
        else if (choice == 1) {
            float w, v, max_time;
            int x, y;
            
            std::cout << "\nОФОРМЛЕНИЕ ЗАКАЗА\n";
            std::cout << "Вес (кг): ";
            std::cin >> w;
            std::cout << "Объем (м3): ";
            std::cin >> v;
            std::cout << "Макс. время доставки: ";
            std::cin >> max_time;
            std::cout << "Координата X: ";
            std::cin >> x;
            std::cout << "Координата Y: ";
            std::cin >> y;

            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: Введены некорректные символы\n";
                continue;
            }

            if (!isValidInput(w, v, x, y, max_time)) {
                std::cout << "Ошибка: Введены неверные значения или превышен лимит (10000)\n";
                continue;
            }

            coords dest = {x, y};
            Order newOrder(order_counter, w, v, dest, max_time); 

            std::cout << "\nРАСПРЕДЕЛЕНИЕ\n";
            
            Transport* best_transport = nullptr;
            float best_time = 999999.0;

            for (Transport* t : fleet) {
                if (t->canHandle(newOrder)) {
                    float current_time = t->calculateTime(newOrder);
                    std::cout << t->getname() << " справится за " << current_time << "\n";
                    
                    if (current_time < best_time) {
                        best_time = current_time;
                        best_transport = t;
                    }
                } else {
                    std::cout << t->getname() << " не подходит\n";
                }
            }

            if (best_transport != nullptr) {
                std::cout << "\nРЕЗУЛЬТАТ: Назначено на " << best_transport->getname() 
                          << " (Время: " << best_time << ")\n";
            } else {
                std::cout << "\nВНИМАНИЕ: Нет доступного транспорта\n";
            }
            
            order_counter++;
        }
        else {
            std::cout << "Неверный ввод\n";
        }
    }

    for (Transport* t : fleet) {
        delete t;
    }
    fleet.clear();

    return 0;
}