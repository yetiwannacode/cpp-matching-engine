#include "OrderBook.hpp"

#include <algorithm>
#include <iostream>

OrderBook::OrderBook() : timestampCounter(0) {}

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

            std::cout << "TRADE: BuyOrder=" << incomingOrder.orderId << " SellOrder=" << restingSellOrder.orderId
                      << " Price=" << restingSellOrder.price << " Quantity=" << tradedQuantity << "\n";

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

            std::cout << "TRADE: BuyOrder=" << restingBuyOrder.orderId << " SellOrder=" << incomingOrder.orderId
                      << " Price=" << restingBuyOrder.price << " Quantity=" << tradedQuantity << "\n";

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