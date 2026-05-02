#include "/home/rexka/uncheba/kurs2sem/repo/Kursovaya/include/order.h" //это пофиксим .json файлом пока пох
#include <iostream>
#include <ctime>
#include <cstdlib>

Order::Order(int id, float w, float v, coords d, int mt): ID(id), weight(w), vol(v), destination(d), max_time(mt) {

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
    std::cout << "ID Закза: " << ID << std::endl;
    std::cout << "Вес: " << weight << std::endl;
    std::cout << "Обьем: " << vol << std::endl;
    std::cout << "Координаты цели доставки: " << destination.x << ", " << destination.y << std::endl;
    std::cout << "Максимальное время доставки: " << max_time << std::endl;
}

int main() {
    int id, mt;
    float w, v;
    coords d = {0, 0};
    srand(time(0));
    id = generateID();
    w = inputWeight();
    v = inputVol();
    mt = inputMaxTime();
    d = inputDestination();
    Order myOrder(id, w, v, d, mt);

    myOrder.PrintStats();
}