#include "/home/rexka/uncheba/kurs2sem/repo/Kursovaya/include/order.h" //это пофиксим .json файлом пока пох
#include <iostream>
#include <ctime>
#include <cstdlib>

Order::Order(int id, int w, int v, coords d, int mt): ID(id), weight(w), vol(v), destination(d), max_time(mt) {

}

void setRandID(int* id) {
    *id = rand() % 90000 + 10000; 
}

void setWeight(int* w) {
    while (true) {
    std::cout << "Введите вес заказа: ";
    std::cin >> *w;
    if (*w <= 0) {
        std::cout << "Введены неверные данные" << std::endl;
    }
    else { 
        break;
    }
    }
}

void setVol(int* v) {
    while (true) {
    std::cout << "Введите обьем заказа: ";
    std::cin >> *v;
    if (*v <= 0) {
        std::cout << "Введены неверные данные" << std::endl;
    }
    else { 
        break;
    }
    }
}

void setMaxTime(int *mt) {
    std::cout << "Введите максимальное время доставки заказа (минуты): ";
    std::cin >> *mt;
    std::cout << std::endl;
}

void Order::PrintStats() {
    std::cout << "ID Закза: " << ID << std::endl;
    std::cout << "Вес: " << weight << std::endl;
    std::cout << "Обьем: " << vol << std::endl;
    std::cout << "Координаты цели доставки: " << destination.x << destination.y << std::endl;
    std::cout << "Максимальное время доставки: " << max_time << std::endl;
}

int main() {
    int id, w, v, mt;
    coords d = {0, 0};
    srand(time(0));
    setRandID(&id);
    setWeight(&w);
    setVol(&v);
    setMaxTime(&mt);
    Order myOrder(id, w, v, d, mt);
    myOrder.PrintStats();
}