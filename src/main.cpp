#include "OrderBook.hpp"

#include <iostream>
#include<stdexcept>
#include <string>

Side parseSide(const std::string& sideText) {
    if (sideText == "BUY") {
        return Side::BUY;
    }

    if (sideText == "SELL") {
        return Side::SELL;
    }

    throw std::invalid_argument("Invalid side. Expected BUY or SELL.");
}

int main() {
    OrderBook orderBook;

    std::string command;

    std::cout << "C++ Matching Engine Simulator\n";
    std::cout << "Supported commands:\n";
    std::cout << "ADD <orderId> <BUY/SELL> <price> <quantity>\n";
    std::cout << "CANCEL <orderId>\n";
    std::cout << "MODIFY <orderId> <newPrice> <newQuantity>\n";
    std::cout << "PRINT\n";
    std::cout << "TRADES\n";
    std::cout << "BENCHMARK <numberOfOrders>\n";
    std::cout << "EXIT\n\n";

    while (std::cin >> command) {
        try {
            if (command == "ADD") {
                int orderId;
                std::string sideText;
                int price;
                int quantity;

                std::cin >> orderId >> sideText >> price >> quantity;

                Side side = parseSide(sideText);
                orderBook.addOrder(orderId, side, price, quantity);
            } else if (command == "CANCEL") {
                int orderId;
                std::cin >> orderId;

                orderBook.cancelOrder(orderId);
            } else if (command == "MODIFY") {
                int orderId;
                int newPrice;
                int newQuantity;

                std::cin >> orderId >> newPrice >> newQuantity;

                orderBook.modifyOrder(orderId, newPrice, newQuantity);
            } else if (command == "PRINT") {
                orderBook.printBook();
            } else if (command == "TRADES") {
                orderBook.printTrades();
            } else if(command == "BENCHMARK"){
                int numberOfOrders;
                std::cin >> numberOfOrders;

                orderBook.runBenchmark(numberOfOrders);
            } else if (command == "EXIT") {
                break;
            } else {
                std::cout << "Unknown command: " << command << "\n";
            }
        } catch (const std::exception& exception) {
            std::cout << "Error: " << exception.what() << "\n";
        }
    }

    return 0;
}