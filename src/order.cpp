#include "order.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cmath>

Order::Order(int id, float w, float v, coords d, int mt, int type): ID(id), weight(w), vol(v), destination(d), max_time(mt), customer_type(type) {

}
void Order::setCustomerType(int type) {
    if (type >= 1 && type <= 3) {
        customer_type = type;
    }
}

void Order::setWeight(float w) {
    if (w > 0) {
        weight = w;
    }
}

void Order::setVol(float v) {
    if (v > 0) {
        vol = v;
    }
}

void Order::setDestination(coords d) {
    destination = d;
}

void Order::setMaxTime(int mt) {
    if (mt > 0) {
        max_time = mt;
    }
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
            std::cout << "Введены неверные данные" << std::endl;
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
            std::cout << "Введены неверные данные" << std::endl;
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
            std::cout << "Введены неверные данные" << std::endl;
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
   std::cout << "ID Заказа: " << ID << std::endl;
    std::cout << "Приоритет: ";
    if (customer_type == 1) std::cout << "VIP" << std::endl;
    else if (customer_type == 2) std::cout << "Экспресс" << std::endl;
    else std::cout << "Эконом" << std::endl;
    
    std::cout << "Вес: " << weight << std::endl;
    std::cout << "Объем: " << vol << std::endl;
    std::cout << "Координаты цели доставки: " << destination.x << ", " << destination.y << std::endl;
    std::cout << "Максимальное время доставки: " << max_time << std::endl;
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
        while (true) {
            std::cout << prompt;
            if (std::cin >> val && val > 0 && val <= 10000) return val;
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Введите число больше 0 и не более 10000.\n";
        }
    }

    int getIntInput(const std::string& prompt) {
        int val;
        while (true) {
            std::cout << prompt;
            if (std::cin >> val && val >= 0 && val <= 10000) return val;
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Введите целое число от 0 до 10000.\n";
        }
    }

    int getStrategyChoice() {
        int strat;
        while (true) {
            std::cout << "\nВЫБЕРИТЕ СТРАТЕГИЮ ДОСТАВКИ:\n"
                      << "1. Быстрая доставка (приоритет времени)\n"
                      << "2. Экономичная доставка (минимальная цена)\n"
                      << "Выбор: ";
            if (std::cin >> strat && (strat == 1 || strat == 2)) return strat;
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Введите 1 или 2.\n";
        }
    }
    int getCustomerTypeChoice() {
        int choice;
        while (true) {
            std::cout << "\nКатегория заказчика:\n"
                      << "1. VIP персона (Максимальный приоритет)\n"
                      << "2. Экспресс доставка\n"
                      << "3. Эконом класс\n"
                      << "Выбор: ";
            if (std::cin >> choice && (choice >= 1 && choice <= 3)) {
                return choice;
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  [Ошибка] Выберите пункт от 1 до 3.\n";
        }
    }
}
