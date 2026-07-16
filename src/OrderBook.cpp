#include "OrderBook.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <vector>

namespace {
struct BenchmarkOrder {
    int orderId;
    Side side;
    int price;
    int quantity;
};
}  // namespace

OrderBook::OrderBook(bool verbose)
    : timestampCounter(0), verboseOutput(verbose) {}

bool OrderBook::addOrder(int orderId, Side side, int price, int quantity) {
    if (quantity <= 0 || price <= 0) {
        if (verboseOutput) {
            std::cout << "Invalid order: price and quantity must be positive.\n";
        }
        return false;
    }

    if (orderMap.find(orderId) != orderMap.end()) {
        if (verboseOutput) {
            std::cout << "Order ID already exists: " << orderId << "\n";
        }
        return false;
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

    return true;
}

void OrderBook::matchBuyOrder(Order& incomingOrder) {
    while (incomingOrder.quantity > 0 && !sellBook.empty()) {
        auto bestSellLevel = sellBook.begin();
        const int bestSellPrice = bestSellLevel->first;

        if (incomingOrder.price < bestSellPrice) {
            break;
        }

        auto& sellQueue = bestSellLevel->second;
        while (incomingOrder.quantity > 0 && !sellQueue.empty()) {
            Order& restingSellOrder = sellQueue.front();
            const int tradedQuantity =
                std::min(incomingOrder.quantity, restingSellOrder.quantity);

            trades.emplace_back(
                incomingOrder.orderId,
                restingSellOrder.orderId,
                restingSellOrder.price,
                tradedQuantity);

            incomingOrder.quantity -= tradedQuantity;
            restingSellOrder.quantity -= tradedQuantity;

            if (verboseOutput) {
                std::cout << "TRADE: BuyOrder=" << incomingOrder.orderId
                          << " SellOrder=" << restingSellOrder.orderId
                          << " Price=" << restingSellOrder.price
                          << " Quantity=" << tradedQuantity << "\n";
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
        const int bestBuyPrice = bestBuyLevel->first;

        if (incomingOrder.price > bestBuyPrice) {
            break;
        }

        auto& buyQueue = bestBuyLevel->second;
        while (incomingOrder.quantity > 0 && !buyQueue.empty()) {
            Order& restingBuyOrder = buyQueue.front();
            const int tradedQuantity =
                std::min(incomingOrder.quantity, restingBuyOrder.quantity);

            trades.emplace_back(
                restingBuyOrder.orderId,
                incomingOrder.orderId,
                restingBuyOrder.price,
                tradedQuantity);

            incomingOrder.quantity -= tradedQuantity;
            restingBuyOrder.quantity -= tradedQuantity;

            if (verboseOutput) {
                std::cout << "TRADE: BuyOrder=" << restingBuyOrder.orderId
                          << " SellOrder=" << incomingOrder.orderId
                          << " Price=" << restingBuyOrder.price
                          << " Quantity=" << tradedQuantity << "\n";
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
        orderMap[order.orderId] =
            OrderLocation{order.side, order.price, iteratorToOrder};
    } else {
        auto& orderQueue = sellBook[order.price];
        orderQueue.push_back(order);
        auto iteratorToOrder = std::prev(orderQueue.end());
        orderMap[order.orderId] =
            OrderLocation{order.side, order.price, iteratorToOrder};
    }
}

void OrderBook::printBook() const {
    std::cout << "\n===== ORDER BOOK =====\n";
    std::cout << "SELL ORDERS:\n";
    for (const auto& priceLevel : sellBook) {
        std::cout << "Price " << priceLevel.first << ": ";
        for (const auto& order : priceLevel.second) {
            std::cout << "[ID=" << order.orderId
                      << ", Qty=" << order.quantity << "] ";
        }
        std::cout << "\n";
    }

    std::cout << "\nBUY ORDERS:\n";
    for (const auto& priceLevel : buyBook) {
        std::cout << "Price " << priceLevel.first << ": ";
        for (const auto& order : priceLevel.second) {
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

bool OrderBook::cancelOrder(int orderId) {
    auto orderLocationIterator = orderMap.find(orderId);
    if (orderLocationIterator == orderMap.end()) {
        if (verboseOutput) {
            std::cout << "Cancel failed: Order ID not found: " << orderId << "\n";
        }
        return false;
    }

    const OrderLocation location = orderLocationIterator->second;
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
    if (verboseOutput) {
        std::cout << "Cancelled order: " << orderId << "\n";
    }
    return true;
}

bool OrderBook::modifyOrder(int orderId, int newPrice, int newQuantity) {
    auto orderLocationIterator = orderMap.find(orderId);
    if (orderLocationIterator == orderMap.end()) {
        if (verboseOutput) {
            std::cout << "Modify failed: Order ID not found: " << orderId << "\n";
        }
        return false;
    }

    if (newPrice <= 0 || newQuantity <= 0) {
        if (verboseOutput) {
            std::cout << "Modify failed: price and quantity must be positive.\n";
        }
        return false;
    }

    const Side existingSide = orderLocationIterator->second.side;
    const bool previousVerbose = verboseOutput;
    verboseOutput = false;
    const bool cancelled = cancelOrder(orderId);
    const bool added = cancelled && addOrder(orderId, existingSide, newPrice, newQuantity);
    verboseOutput = previousVerbose;

    if (!added) {
        return false;
    }

    if (verboseOutput) {
        std::cout << "Modified order: " << orderId
                  << " NewPrice=" << newPrice
                  << " NewQuantity=" << newQuantity << "\n";
    }
    return true;
}

OrderBook::BenchmarkResult OrderBook::runBenchmarkOnce(
    int numberOfOrders,
    unsigned int seed) {
    std::mt19937 randomGenerator(seed);
    std::uniform_int_distribution<int> sideDistribution(0, 1);
    std::uniform_int_distribution<int> priceDistribution(90, 110);
    std::uniform_int_distribution<int> quantityDistribution(1, 100);

    std::vector<BenchmarkOrder> generatedOrders;
    generatedOrders.reserve(static_cast<std::size_t>(numberOfOrders));

    for (int orderId = 1; orderId <= numberOfOrders; ++orderId) {
        generatedOrders.push_back(BenchmarkOrder{
            orderId,
            sideDistribution(randomGenerator) == 0 ? Side::BUY : Side::SELL,
            priceDistribution(randomGenerator),
            quantityDistribution(randomGenerator)});
    }

    OrderBook benchmarkBook(false);
    const auto startTime = std::chrono::steady_clock::now();

    for (const auto& order : generatedOrders) {
        benchmarkBook.addOrder(
            order.orderId, order.side, order.price, order.quantity);
    }

    const auto endTime = std::chrono::steady_clock::now();
    const auto totalMicroseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime)
            .count();

    const double averageMicroseconds =
        static_cast<double>(totalMicroseconds) /
        static_cast<double>(numberOfOrders);

    return BenchmarkResult{
        totalMicroseconds,
        averageMicroseconds,
        benchmarkBook.trades.size()};
}

void OrderBook::runBenchmark(int numberOfOrders) {
    if (numberOfOrders <= 0) {
        std::cout << "Benchmark failed: number of orders must be positive.\n";
        return;
    }

    const BenchmarkResult result = runBenchmarkOnce(numberOfOrders, 42U);
    std::cout << "\n===== BENCHMARK =====\n";
    std::cout << "Processed orders: " << numberOfOrders << "\n";
    std::cout << "Total time: " << result.totalMicroseconds << " microseconds\n";
    std::cout << "Average processing time: " << std::fixed
              << std::setprecision(4) << result.averageMicroseconds
              << " microseconds/order\n";
    std::cout << "Trades generated: " << result.tradesGenerated << "\n";
    std::cout << "=====================\n";
}

void OrderBook::runBenchmarkMultiple(int numberOfOrders, int runs) {
    if (numberOfOrders <= 0 || runs <= 0) {
        std::cout << "Benchmark failed: number of orders and runs must be positive.\n";
        return;
    }

    long long bestTime = std::numeric_limits<long long>::max();
    long long worstTime = 0;
    long long totalTime = 0;
    std::size_t totalTrades = 0;

    for (int run = 0; run < runs; ++run) {
        const BenchmarkResult result = runBenchmarkOnce(numberOfOrders, 42U);
        bestTime = std::min(bestTime, result.totalMicroseconds);
        worstTime = std::max(worstTime, result.totalMicroseconds);
        totalTime += result.totalMicroseconds;
        totalTrades += result.tradesGenerated;
    }

    const double averageTotalTime =
        static_cast<double>(totalTime) / static_cast<double>(runs);
    const double averagePerOrder =
        averageTotalTime / static_cast<double>(numberOfOrders);
    const double averageTrades =
        static_cast<double>(totalTrades) / static_cast<double>(runs);

    std::cout << "\n===== MULTI-RUN BENCHMARK =====\n";
    std::cout << "Runs: " << runs << "\n";
    std::cout << "Orders per run: " << numberOfOrders << "\n";
    std::cout << "Best time: " << bestTime << " microseconds\n";
    std::cout << "Average time: " << std::fixed << std::setprecision(2)
              << averageTotalTime << " microseconds\n";
    std::cout << "Worst time: " << worstTime << " microseconds\n";
    std::cout << "Average processing time: " << std::fixed
              << std::setprecision(4) << averagePerOrder
              << " microseconds/order\n";
    std::cout << "Average trades generated: " << std::fixed
              << std::setprecision(2) << averageTrades << "\n";
    std::cout << "===============================\n";
}

bool OrderBook::hasOrder(int orderId) const noexcept {
    return orderMap.find(orderId) != orderMap.end();
}

std::optional<Order> OrderBook::getOrder(int orderId) const {
    const auto location = orderMap.find(orderId);
    if (location == orderMap.end()) {
        return std::nullopt;
    }
    return *(location->second.iterator);
}

const std::vector<Trade>& OrderBook::getTrades() const noexcept {
    return trades;
}

std::size_t OrderBook::activeOrderCount() const noexcept {
    return orderMap.size();
}

std::optional<int> OrderBook::bestBid() const noexcept {
    if (buyBook.empty()) {
        return std::nullopt;
    }
    return buyBook.begin()->first;
}

std::optional<int> OrderBook::bestAsk() const noexcept {
    if (sellBook.empty()) {
        return std::nullopt;
    }
    return sellBook.begin()->first;
}
