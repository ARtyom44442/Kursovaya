#include "order.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <string>

Order::Order(int id, float w, float v, coords d, int mt, int type): ID(id), weight(w), vol(v), destination(d), max_time(mt), customer_type(type) {}

void Order::setCustomerType(int type) {
    if (type >= 1 && type <= 3) customer_type = type;
}

void Order::setWeight(float w) {
    if (w > 0) weight = w;
}

void Order::setVol(float v) {
    if (v > 0) vol = v;
}

void Order::setDestination(coords d) {
    destination = d;
}

void Order::setMaxTime(int mt) {
    if (mt > 0) max_time = mt;
}

int generateID() {
    return rand() % 90000 + 10000; 
}

float inputWeight() {
    float w;
    while (true) {
        std::cout << "Введите вес заказа: ";
        std::cin >> w;
        if (w <= 0) {
            std::cout << "Введены неверные данные\n";
        }
        else { 
            return w;
        }
    }
}

float inputVol() {
    float v;
    while (true) {
        std::cout << "Введите обьем заказа: ";
        std::cin >> v;
        if (v <= 0) {
            std::cout << "Введены неверные данные\n";
        }
        else { 
            return v;
        }
    }
}

int inputMaxTime() {
    int mt;
    while (true){
        std::cout << "Введите максимальное время доставки заказа (минуты): ";
        std::cin >> mt;
        if (mt <= 0) {
            std::cout << "Введены неверные данные\n";
        }
        else {
            return mt;
        }
    }
}

coords inputDestination() {
    coords d;
    std::cout << "Введите значение X координат места доставки: ";
    std::cin >> d.x;
    std::cout << "Введите значение Y координат места доставки: ";
    std::cin >> d.y;
    return d;
}

void Order::PrintStats() {
    std::cout << "ID Заказа: " << ID << "\n";
    std::cout << "Приоритет: ";
    if (customer_type == 1) std::cout << "VIP\n";
    else if (customer_type == 2) std::cout << "Экспресс\n";
    else std::cout << "Эконом\n";
    
    std::cout << "Вес: " << weight << "\n";
    std::cout << "Объем: " << vol << "\n";
    std::cout << "Координаты цели доставки: " << destination.x << ", " << destination.y << "\n";
    std::cout << "Максимальное время доставки: " << max_time << "\n";
}

bool Order::areOrdersClose(const std::vector<Order>& orders, float threshold) {
    if (orders.size() < 2) return true;
    for (size_t i = 0; i < orders.size(); ++i) {
        for (size_t j = i + 1; j < orders.size(); ++j) {
            coords a = orders[i].getDestination();
            coords b = orders[j].getDestination();
            float dist = std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
            if (dist > threshold) return false;
        }
    }
    return true;
}

namespace UI {
    float getFloatInput(const std::string& prompt) {
        float val;
        std::string input;
        while (true) {
            std::cout << prompt;
            if (std::cin >> input) {
                try {
                    size_t pos;
                    val = std::stof(input, &pos);
                    if (pos == input.length() && val > 0 && val <= 10000) return val;
                } catch (...) {}
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Введите число больше 0 и не более 10000.\n";
        }
    }

    int getIntInput(const std::string& prompt) {
        int val;
        std::string input;
        while (true) {
            std::cout << prompt;
            if (std::cin >> input) {
                try {
                    size_t pos;
                    val = std::stoi(input, &pos);
                    if (pos == input.length() && val >= 0 && val <= 10000) return val;
                } catch (...) {}
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Введите целое число от 0 до 10000.\n";
        }
    }

    int getCoordInput(const std::string& prompt) {
        int val;
        std::string input;
        while (true) {
            std::cout << prompt;
            if (std::cin >> input) {
                try {
                    size_t pos;
                    val = std::stoi(input, &pos);
                    if (pos == input.length() && val >= -10000 && val <= 10000) return val;
                } catch (...) {}
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Введите координату от -10000 до 10000.\n";
        }
    }

    int getStrategyChoice() {
        int strat;
        while (true) {
            std::cout << "\nВыберите стратегию доставки:\n"
                      << "1. Быстрая доставка (приоритет времени)\n"
                      << "2. Экономичная доставка (минимальная цена)\n"
                      << "Выбор: ";
            if (std::cin >> strat && (strat == 1 || strat == 2)) {
                std::cin.ignore(10000, '\n');
                return strat;
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Введите 1 или 2.\n";
        }
    }
    int getCustomerTypeChoice() {
        int choice;
        while (true) {
            std::cout << "\nКатегория заказчика:\n"
                      << "1. VIP персона (максимальный приоритет)\n"
                      << "2. Экспресс доставка\n"
                      << "3. Эконом класс\n"
                      << "Выбор: ";
            if (std::cin >> choice && (choice >= 1 && choice <= 3)) {
                std::cin.ignore(10000, '\n');
                return choice;
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Выберите пункт от 1 до 3.\n";
        }
    }
    int getOrderIdForDeletion() {
        int id;
        while (true) {
            std::cout << "Введите ID заказа для удаления (или 0 для отмены): ";
            if (std::cin >> id && id >= 0) {
                std::cin.ignore(10000, '\n');
                return id;
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Введите корректный ID.\n";
        }
    }
}