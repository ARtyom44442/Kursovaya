#include <iostream>
#include <vector>
#include <limits>
#include <clocale>
#include <string>
#include <sstream>
#include <windows.h>
#include <iomanip>
#include <cmath>
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

std::string formatTime(float hours) {
    std::stringstream ss;
    int h = static_cast<int>(hours);
    int m = static_cast<int>((hours - h) * 60 + 0.5f);
    if (m >= 60) {
        h++;
        m = 0;
    }
    if (h > 0) {
        ss << h << " час. ";
    }
    if (m > 0 || h == 0) {
        ss << m << " мин.";
    }
    return ss.str();
}

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    setlocale(LC_ALL, "Russian_Russia.65001");

    DataReader reader;
    std::vector<Transport*> fleet = reader.loadTransports("data/transports.json");

    if (fleet.empty()) {
        std::cerr << "Ошибка: Автопарк пуст или файл не найден\n";
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
        else if (choice == 1) {
            float w, v, max_time;
            int x, y;
            
            std::cout << "\nОФОРМЛЕНИЕ ЗАКАЗА\n";

            std::cout << "Вес (кг): ";
            if (!(std::cin >> w) || w < 0) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: Вес не может быть отрицательным или текстом\n";
                continue;
            }

            std::cout << "Объем (м3): ";
            if (!(std::cin >> v) || v < 0) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: Объем не может быть отрицательным или текстом\n";
                continue;
            }

            std::cout << "Макс. время доставки (часы): ";
            if (!(std::cin >> max_time) || max_time < 0) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: Время не может быть отрицательным или текстом\n";
                continue;
            }

            std::cout << "Координата X: ";
            if (!(std::cin >> x) || x < 0) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: Координата X не может быть отрицательной\n";
                continue;
            }

            std::cout << "Координата Y: ";
            if (!(std::cin >> y) || y < 0) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: Координата Y не может быть отрицательной\n";
                continue;
            }

            if (!isValidInput(w, v, x, y, max_time)) {
                std::cout << "Ошибка: Превышен лимит (10000)\n";
                continue;
            }

            coords dest = {x, y};
            int max_time_minutes = static_cast<int>(max_time * 60);
            Order newOrder(order_counter, w, v, dest, max_time_minutes); 

            std::cout << "\nРАСПРЕДЕЛЕНИЕ\n";
            
            Transport* best_transport = nullptr;
            float best_time = 99999; 

            for (Transport* t : fleet) {
                if (!isLogicalChoice(t->getname(), w, v, x, y)) {
                    std::cout << t->getname() << " не подходит (нерентабельно)\n";
                    continue;
                }

                if (t->canHandle(newOrder)) {
                    float current_time = t->calculateTime(newOrder);
                    
                    if (current_time > max_time) {
                        std::cout << t->getname() << " не успеет (нужно: " << formatTime(current_time) << ")\n";
                        continue;
                    }

                    std::cout << t->getname() << " справится за " << formatTime(current_time) << "\n";
                    
                    if (current_time < best_time) {
                        best_time = current_time;
                        best_transport = t;
                    }
                } else {
                    std::cout << t->getname() << " не подходит по габаритам\n";
                }
            }

            if (best_transport != nullptr) {
                std::cout << "\nРЕЗУЛЬТАТ: Назначено на " << best_transport->getname() 
                          << " (Время: " << formatTime(best_time) << ")\n";
                float price = best_transport->calculatePrice(newOrder);
                std::cout << "Стоимость доставки: " << std::fixed << std::setprecision(2) << price << " руб.\n";
            } else {
                std::cout << "\nВНИМАНИЕ: Нет доступного транспорта для этого заказа\n";
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
