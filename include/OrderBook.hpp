#pragma once

#include "Order.hpp"
#include "Trade.hpp"

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

class OrderBook {
private:
    using Price = int;
    using OrderQueue = std::list<Order>;
    using BuyBook = std::map<Price, OrderQueue, std::greater<Price>>;
    using SellBook = std::map<Price, OrderQueue>;

    BuyBook buyBook;
    SellBook sellBook;

    struct OrderLocation {
        Side side;
        Price price;
        OrderQueue::iterator iterator;
    };

    struct BenchmarkResult {
        long long totalMicroseconds;
        double averageMicroseconds;
        std::size_t tradesGenerated;
    };

    std::unordered_map<int, OrderLocation> orderMap;
    std::vector<Trade> trades;
    long long timestampCounter;
    bool verboseOutput;

    void matchBuyOrder(Order& incomingOrder);
    void matchSellOrder(Order& incomingOrder);
    void addToBook(const Order& order);
    BenchmarkResult runBenchmarkOnce(int numberOfOrders, unsigned int seed);

public:
    explicit OrderBook(bool verbose = true);

    bool addOrder(int orderId, Side side, int price, int quantity);
    bool cancelOrder(int orderId);
    bool modifyOrder(int orderId, int newPrice, int newQuantity);

    void printBook() const;
    void printTrades() const;
    void runBenchmark(int numberOfOrders);
    void runBenchmarkMultiple(int numberOfOrders, int runs);

    [[nodiscard]] bool hasOrder(int orderId) const noexcept;
    [[nodiscard]] std::optional<Order> getOrder(int orderId) const;
    [[nodiscard]] const std::vector<Trade>& getTrades() const noexcept;
    [[nodiscard]] std::size_t activeOrderCount() const noexcept;
    [[nodiscard]] std::optional<int> bestBid() const noexcept;
    [[nodiscard]] std::optional<int> bestAsk() const noexcept;
};
