#include "OrderBook.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>

OrderBook::OrderBook() : timestampCounter(0), verboseOutput(true) {}

void OrderBook::addOrder(int orderId, Side side, int price, int quantity) {
    if (quantity <= 0 || price <= 0) {
        std::cout << "Invalid order: price and quantity must be positive.\n";
        return;
    }

    if (orderMap.find(orderId) != orderMap.end()) {
        std::cout << "Order ID already exists: " << orderId << "\n";
        return;
    }

    Order incomingOrder(orderId, side, price, quantity, ++timestampCounter);

    if (side == Side::BUY) {
        matchBuyOrder(incomingOrder);
    } else {
        matchSellOrder(incomingOrder);
    }

    if (incomingOrder.quantity > 0) {
        addToBook(incomingOrder);
    }
}

void OrderBook::matchBuyOrder(Order& incomingOrder) {
    while (incomingOrder.quantity > 0 && !sellBook.empty()) {
        auto bestSellLevel = sellBook.begin();
        int bestSellPrice = bestSellLevel->first;

        if (incomingOrder.price < bestSellPrice) {
            break;
        }

        auto& sellQueue = bestSellLevel->second;

        while (incomingOrder.quantity > 0 && !sellQueue.empty()) {
            Order& restingSellOrder = sellQueue.front();

            int tradedQuantity = std::min(incomingOrder.quantity, restingSellOrder.quantity);

            trades.emplace_back(
                incomingOrder.orderId,
                restingSellOrder.orderId,
                restingSellOrder.price,
                tradedQuantity
            );

            incomingOrder.quantity -= tradedQuantity;
            restingSellOrder.quantity -= tradedQuantity;

            if(verboseOutput){
                std::cout << "TRADE: BuyOrder=" << incomingOrder.orderId << " SellOrder=" << restingSellOrder.orderId
                          << " Price=" << restingSellOrder.price << " Quantity=" << tradedQuantity << "\n";
            }

            if (restingSellOrder.quantity == 0) {
                orderMap.erase(restingSellOrder.orderId);
                sellQueue.pop_front();
            }
        }

        if (sellQueue.empty()) {
            sellBook.erase(bestSellLevel);
        }
    }
}

void OrderBook::matchSellOrder(Order& incomingOrder) {
    while (incomingOrder.quantity > 0 && !buyBook.empty()) {
        auto bestBuyLevel = buyBook.begin();
        int bestBuyPrice = bestBuyLevel->first;

        if (incomingOrder.price > bestBuyPrice) {
            break;
        }

        auto& buyQueue = bestBuyLevel->second;

        while (incomingOrder.quantity > 0 && !buyQueue.empty()) {
            Order& restingBuyOrder = buyQueue.front();

            int tradedQuantity = std::min(incomingOrder.quantity, restingBuyOrder.quantity);

            trades.emplace_back(
                restingBuyOrder.orderId,
                incomingOrder.orderId,
                restingBuyOrder.price,
                tradedQuantity
            );

            incomingOrder.quantity -= tradedQuantity;
            restingBuyOrder.quantity -= tradedQuantity;

            if(verboseOutput){
                std::cout << "TRADE: BuyOrder=" << restingBuyOrder.orderId << " SellOrder=" << incomingOrder.orderId
                          << " Price=" << restingBuyOrder.price << " Quantity=" << tradedQuantity << "\n";
            }
            
            if (restingBuyOrder.quantity == 0) {
                orderMap.erase(restingBuyOrder.orderId);
                buyQueue.pop_front();
            }
        }

        if (buyQueue.empty()) {
            buyBook.erase(bestBuyLevel);
        }
    }
}

void OrderBook::addToBook(const Order& order) {
    if (order.side == Side::BUY) {
        auto& orderQueue = buyBook[order.price];
        orderQueue.push_back(order);

        auto iteratorToOrder = std::prev(orderQueue.end());

        orderMap[order.orderId] = OrderLocation{
            order.side,
            order.price,
            iteratorToOrder
        };
    } else {
        auto& orderQueue = sellBook[order.price];
        orderQueue.push_back(order);

        auto iteratorToOrder = std::prev(orderQueue.end());

        orderMap[order.orderId] = OrderLocation{
            order.side,
            order.price,
            iteratorToOrder
        };
    }
}

void OrderBook::printBook() const {
    std::cout << "\n===== ORDER BOOK =====\n";

    std::cout << "SELL ORDERS:\n";
    for (const auto& priceLevel : sellBook) {
        int price = priceLevel.first;
        const auto& orderQueue = priceLevel.second;

        std::cout << "Price " << price << ": ";
        for (const auto& order : orderQueue) {
            std::cout << "[ID=" << order.orderId
                      << ", Qty=" << order.quantity << "] ";
        }
        std::cout << "\n";
    }

    std::cout << "\nBUY ORDERS:\n";
    for (const auto& priceLevel : buyBook) {
        int price = priceLevel.first;
        const auto& orderQueue = priceLevel.second;

        std::cout << "Price " << price << ": ";
        for (const auto& order : orderQueue) {
            std::cout << "[ID=" << order.orderId
                      << ", Qty=" << order.quantity << "] ";
        }
        std::cout << "\n";
    }

    std::cout << "======================\n";
}

void OrderBook::printTrades() const {
    std::cout << "\n===== TRADES =====\n";

    if (trades.empty()) {
        std::cout << "No trades executed.\n";
    }

    for (const auto& trade : trades) {
        std::cout << "BuyOrder=" << trade.buyOrderId
                  << " SellOrder=" << trade.sellOrderId
                  << " Price=" << trade.price
                  << " Quantity=" << trade.quantity << "\n";
    }

    std::cout << "==================\n";
}

void OrderBook::cancelOrder(int orderId) {
    auto orderLocationIterator = orderMap.find(orderId);

    if (orderLocationIterator == orderMap.end()) {
        std::cout << "Cancel failed: Order ID not found: " << orderId << "\n";
        return;
    }

    OrderLocation location = orderLocationIterator->second;

    if (location.side == Side::BUY) {
        auto priceLevelIterator = buyBook.find(location.price);

        if (priceLevelIterator != buyBook.end()) {
            auto& orderQueue = priceLevelIterator->second;
            orderQueue.erase(location.iterator);

            if (orderQueue.empty()) {
                buyBook.erase(priceLevelIterator);
            }
        }
    } else {
        auto priceLevelIterator = sellBook.find(location.price);

        if (priceLevelIterator != sellBook.end()) {
            auto& orderQueue = priceLevelIterator->second;
            orderQueue.erase(location.iterator);

            if (orderQueue.empty()) {
                sellBook.erase(priceLevelIterator);
            }
        }
    }

    orderMap.erase(orderId);

    std::cout << "Cancelled order: " << orderId << "\n";
}

void OrderBook::modifyOrder(int orderId, int newPrice, int newQuantity) {
    auto orderLocationIterator = orderMap.find(orderId);

    if (orderLocationIterator == orderMap.end()) {
        std::cout << "Modify failed: Order ID not found: " << orderId << "\n";
        return;
    }

    if (newPrice <= 0 || newQuantity <= 0) {
        std::cout << "Modify failed: price and quantity must be positive.\n";
        return;
    }

    Side existingSide = orderLocationIterator->second.side;

    cancelOrder(orderId);
    addOrder(orderId, existingSide, newPrice, newQuantity);

    std::cout << "Modified order: " << orderId
              << " NewPrice=" << newPrice
              << " NewQuantity=" << newQuantity << "\n";
}

void OrderBook::runBenchmark(int numberOfOrders) {
    if (numberOfOrders <= 0) {
        std::cout << "Benchmark failed: number of orders must be positive.\n";
        return;
    }

    OrderBook benchmarkBook;
    benchmarkBook.verboseOutput = false;

    std::mt19937 randomGenerator(42);
    std::uniform_int_distribution<int> sideDistribution(0, 1);
    std::uniform_int_distribution<int> priceDistribution(90, 110);
    std::uniform_int_distribution<int> quantityDistribution(1, 100);

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int orderId = 1; orderId <= numberOfOrders; ++orderId) {
        Side side = sideDistribution(randomGenerator) == 0 ? Side::BUY : Side::SELL;
        int price = priceDistribution(randomGenerator);
        int quantity = quantityDistribution(randomGenerator);

        benchmarkBook.addOrder(orderId, side, price, quantity);
    }

    auto endTime = std::chrono::high_resolution_clock::now();

    auto totalMicroseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    double averageMicroseconds =
        static_cast<double>(totalMicroseconds) / static_cast<double>(numberOfOrders);

    std::cout << "\n===== BENCHMARK =====\n";
    std::cout << "Processed orders: " << numberOfOrders << "\n";
    std::cout << "Total time: " << totalMicroseconds << " microseconds\n";
    std::cout << "Average processing time: "
              << std::fixed << std::setprecision(4)
              << averageMicroseconds << " microseconds/order\n";
    std::cout << "Trades generated: " << benchmarkBook.trades.size() << "\n";
    std::cout << "=====================\n";
}