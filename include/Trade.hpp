#pragma once

struct Trade {
    int buyOrderId;
    int sellOrderId;
    int price;
    int quantity;

    Trade(int buyId, int sellId, int tradePrice, int tradeQuantity)
        : buyOrderId(buyId),
          sellOrderId(sellId),
          price(tradePrice),
          quantity(tradeQuantity) {}
};