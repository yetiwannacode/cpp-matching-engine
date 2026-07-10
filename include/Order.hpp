#pragma once
#include <string>

enum class Side {
    BUY,
    SELL
};

inline std::string sideToString(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

struct Order {
    int orderId;
    Side side;
    int price;
    int quantity;
    long long timestamp;

    Order(int id, Side orderSide, int orderPrice, int orderQuantity, long long orderTimestamp)
        : orderId(id),
          side(orderSide),
          price(orderPrice),
          quantity(orderQuantity),
          timestamp(orderTimestamp) {}
};