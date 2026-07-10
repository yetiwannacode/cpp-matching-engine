#include "OrderBook.hpp"

#include <iostream>

int main() {
    OrderBook orderBook;

    std::cout << "Adding sample orders...\n";

    orderBook.addOrder(1, Side::BUY, 100, 10);
    orderBook.addOrder(2, Side::SELL, 99, 5);
    orderBook.addOrder(3, Side::SELL, 101, 7);
    orderBook.addOrder(4, Side::BUY, 101, 10);

    orderBook.printBook();
    orderBook.printTrades();

    return 0;
}