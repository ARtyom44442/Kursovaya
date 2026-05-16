#include <iostream>
#include <vector>
#include <limits>
#include <clocale>
#include <string>
#include <sstream>
#include <windows.h>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include "order.h"
#include "datareader.h"
#include "Transport.h"

const float PROXIMITY_THRESHOLD = 50.0f;

Transport* findFastestTransport(const std::vector<Transport*>& fleet, const Order& order) {
    Transport* best = nullptr;
    float bestTime = std::numeric_limits<float>::max();

    for (Transport* t : fleet) {
        if (!t->canHandle(const_cast<Order&>(order)))  
            continue;

        float timeNeeded = t->calculateTime(const_cast<Order&>(order));

        if (timeNeeded > order.getMaxTime())
            continue;

        if (timeNeeded < bestTime) {
            bestTime = timeNeeded;
            best = t;
        }
    }
    return best;
}

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

bool areOrdersClose(const std::vector<Order>& orders, float threshold) {
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

struct RouteInfo {
    Transport* transport;
    std::vector<int> orderIndices;
    std::vector<float> arrivalTimes;
    float totalTime;
};

bool buildRoute(Transport* t, const std::vector<Order>& orders, RouteInfo& route) {
    float totalWeight = 0, totalVol = 0;
    for (const auto& ord : orders) {
        totalWeight += ord.getWeight();
        totalVol += ord.getVol();
    }
    if (totalWeight > t->getmax_w() || totalVol > t->getmax_v()) return false;

    coords currentPos = t->getCurrentPos();
    std::vector<bool> delivered(orders.size(), false);
    std::vector<int> orderIdx;
    std::vector<float> times;
    float elapsed = 0.0f;

    for (size_t k = 0; k < orders.size(); ++k) {
        int bestIdx = -1;
        float bestDist = std::numeric_limits<float>::max();
        for (size_t i = 0; i < orders.size(); ++i) {
            if (delivered[i]) continue;
            coords dest = orders[i].getDestination();
            float dist = std::sqrt((dest.x - currentPos.x) * (dest.x - currentPos.x) +
                (dest.y - currentPos.y) * (dest.y - currentPos.y));
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = static_cast<int>(i);
            }
        }
        if (bestIdx == -1) break;

        float travelTime = bestDist / t->getspeed();
        elapsed += travelTime;
        orderIdx.push_back(bestIdx);
        times.push_back(elapsed);

        currentPos = orders[bestIdx].getDestination();
        delivered[bestIdx] = true;
    }

    route.transport = t;
    route.orderIndices = orderIdx;
    route.arrivalTimes = times;
    route.totalTime = elapsed;
    return true;
}

void performGroupDelivery(const std::vector<Transport*>& fleet, std::vector<Order>& groupOrders) {
    if (groupOrders.empty()) {
        std::cout << "Нет заказов в группе. Сначала добавьте заказы (пункт 3).\n";
        return;
    }

    if (!areOrdersClose(groupOrders, PROXIMITY_THRESHOLD)) {
        std::cout << "Заказы расположены слишком далеко друг от друга (более " << PROXIMITY_THRESHOLD << " км). Группировка невозможна.\n";
        return;
    }

    std::vector<RouteInfo> validRoutes;
    for (Transport* t : fleet) {
        RouteInfo route;
        if (buildRoute(t, groupOrders, route)) {
            validRoutes.push_back(route);
        }
        else {
            std::cout << t->getname() << " не может перевезти суммарный вес/объём.\n";
        }
    }

    if (validRoutes.empty()) {
        std::cout << "Нет подходящего транспорта для группировки этих заказов.\n";
        return;
    }

    std::cout << "\n=== РАСЧЁТ ДЛЯ КАЖДОГО ТРАНСПОРТА ===\n";
    for (const auto& route : validRoutes) {
        std::cout << "\n--- " << route.transport->getname()
            << " (Скорость: " << route.transport->getspeed() << " км/ч) ---\n";
        std::cout << "Последовательность доставки:\n";
        for (size_t i = 0; i < route.orderIndices.size(); ++i) {
            int idx = route.orderIndices[i];
            const Order& ord = groupOrders[idx];
            std::cout << "  " << (i + 1) << ". Заказ #" << ord.getID()
                << " (координаты: " << ord.getDestination().x << ", " << ord.getDestination().y << ")"
                << " — время прибытия: " << formatTime(route.arrivalTimes[i]) << "\n";
        }
        std::cout << "Общее время доставки всех заказов: " << formatTime(route.totalTime) << "\n";
    }

    auto best = std::min_element(validRoutes.begin(), validRoutes.end(),
        [](const RouteInfo& a, const RouteInfo& b) { return a.totalTime < b.totalTime; });

    std::cout << "\n=== ЛУЧШИЙ ТРАНСПОРТ ===\n";
    std::cout << "Назначен: " << best->transport->getname() << "\n";
    std::cout << "Последовательность доставки:\n";
    for (size_t i = 0; i < best->orderIndices.size(); ++i) {
        int idx = best->orderIndices[i];
        const Order& ord = groupOrders[idx];
        std::cout << "  " << (i + 1) << ". Заказ #" << ord.getID()
            << " — время прибытия: " << formatTime(best->arrivalTimes[i]) << "\n";
    }
    std::cout << "Общее время: " << formatTime(best->totalTime) << "\n";
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
    std::vector<Order> groupOrders;

    while (true) {
        std::cout << "\nГЛАВНОЕ МЕНЮ\n";
        std::cout << "1. Создать новый заказ (одиночный, сразу назначается транспорт)\n";
        std::cout << "2. Посмотреть доступный транспорт\n";
        std::cout << "3. Добавить заказ в группу\n";
        std::cout << "4. Выполнить группировку и доставку\n";
        std::cout << "5. Быстрая доставка (приоритет скорости)\n";   
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
                std::cout << " - " << t->getname()
                    << " (Скорость: " << t->getspeed() << " км/ч, "
                    << "макс. вес: " << t->getmax_w() << " кг, "
                    << "объём: " << t->getmax_v() << " м³)\n";
            }
        }
        else if (choice == 3) {
            float w, v, max_time;
            int x, y;

            std::cout << "\nОФОРМЛЕНИЕ ЗАКАЗА ДЛЯ ГРУППЫ\n";
            std::cout << "Вес (кг): ";
            if (!(std::cin >> w) || w <= 0 || w > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: вес должен быть от 0 до 10000\n";
                continue;
            }
            std::cout << "Объем (м3): ";
            if (!(std::cin >> v) || v <= 0 || v > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: объём должен быть от 0 до 10000\n";
                continue;
            }
            std::cout << "Макс. время доставки (часы): ";
            if (!(std::cin >> max_time) || max_time <= 0 || max_time > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: время должно быть от 0 до 10000\n";
                continue;
            }
            std::cout << "Координата X: ";
            if (!(std::cin >> x) || x < 0 || x > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: X от 0 до 10000\n";
                continue;
            }
            std::cout << "Координата Y: ";
            if (!(std::cin >> y) || y < 0 || y > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: Y от 0 до 10000\n";
                continue;
            }

            coords dest = { x, y };
            int max_time_minutes = static_cast<int>(max_time * 60);
            Order newOrder(order_counter, w, v, dest, max_time_minutes);
            groupOrders.push_back(newOrder);
            std::cout << "Заказ #" << order_counter << " добавлен в группу. Всего заказов в группе: " << groupOrders.size() << "\n";
            order_counter++;
        }
        else if (choice == 4) {
            performGroupDelivery(fleet, groupOrders);
            groupOrders.clear();
        }
        else if (choice == 5) {
            float w, v, max_time;
            int x, y;

            std::cout << "\n=== БЫСТРАЯ ДОСТАВКА (ПРИОРИТЕТ СКОРОСТИ) ===\n";
            std::cout << "Вес (кг): ";
            if (!(std::cin >> w) || w <= 0 || w > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: вес должен быть от 0 до 10000\n";
                continue;
            }
            std::cout << "Объем (м3): ";
            if (!(std::cin >> v) || v <= 0 || v > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: объём должен быть от 0 до 10000\n";
                continue;
            }
            std::cout << "Макс. время доставки (часы): ";
            if (!(std::cin >> max_time) || max_time <= 0 || max_time > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: время должно быть от 0 до 10000\n";
                continue;
            }
            std::cout << "Координата X: ";
            if (!(std::cin >> x) || x < 0 || x > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: X от 0 до 10000\n";
                continue;
            }
            std::cout << "Координата Y: ";
            if (!(std::cin >> y) || y < 0 || y > 10000) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка: Y от 0 до 10000\n";
                continue;
            }

            coords dest = { x, y };
            int max_time_minutes = static_cast<int>(max_time * 60);
            Order newOrder(order_counter, w, v, dest, max_time_minutes);

            Transport* fastest = findFastestTransport(fleet, newOrder);
            if (fastest != nullptr) {
                float timeNeeded = fastest->calculateTime(newOrder);
                std::cout << "\nРЕЗУЛЬТАТ (стратегия «максимальная скорость»):\n";
                std::cout << "Назначен транспорт: " << fastest->getname() << "\n";
                std::cout << "Время доставки: " << formatTime(timeNeeded) << "\n";
            }
            else {
                std::cout << "\nНет подходящего транспорта для этого заказа.\n";
            }
            order_counter++;
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

            coords dest = { x, y };
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
                }
                else {
                    std::cout << t->getname() << " не подходит по габаритам\n";
                }
            }

            if (best_transport != nullptr) {
                std::cout << "\nРЕЗУЛЬТАТ: Назначено на " << best_transport->getname()
                    << " (Время: " << formatTime(best_time) << ")\n";
            }
            else {
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