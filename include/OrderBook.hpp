#pragma once

#include "Order.hpp"
#include "Trade.hpp"

#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

class OrderBook {
private:
    using Price = int;
    using OrderQueue = std::list<Order>;
    using BuyBook = std::map<Price, OrderQueue, std::greater<Price>>;
    using SellBook = std::map<Price, OrderQueue>;

    // Buy book: highest price first
    BuyBook buyBook;

    // Sell book: lowest price first
    SellBook sellBook;

    // Stores where an order currently exists in the book
    struct OrderLocation {
        Side side;
        Price price;
        OrderQueue::iterator iterator;
    };

    // Fast lookup by order ID for cancel/modify
    std::unordered_map<int, OrderLocation> orderMap;

    // Stores all executed trades
    std::vector<Trade> trades;

    // Used to preserve time priority
    long long timestampCounter;

    // Internal helper functions
    void matchBuyOrder(Order& incomingOrder);
    void matchSellOrder(Order& incomingOrder);
    void addToBook(const Order& order);

public:
    OrderBook();

    void addOrder(int orderId, Side side, int price, int quantity);
    void cancelOrder(int orderId);
    void modifyOrder(int orderId, int newPrice, int newQuantity);
    
    void printBook() const;
    void printTrades() const;
    void runBenchmark(int numberOfOrders);
};