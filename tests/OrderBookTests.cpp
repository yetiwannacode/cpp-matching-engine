#include "OrderBook.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testBetterPriceExecutesFirst() {
    OrderBook book(false);
    book.addOrder(1, Side::SELL, 101, 5);
    book.addOrder(2, Side::SELL, 100, 5);
    book.addOrder(3, Side::BUY, 101, 5);

    const auto& trades = book.getTrades();
    expect(trades.size() == 1, "Expected exactly one trade");
    expect(trades[0].sellOrderId == 2, "Best ask should execute first");
    expect(trades[0].price == 100, "Trade should occur at resting price");
}

void testFifoWithinPriceLevel() {
    OrderBook book(false);
    book.addOrder(1, Side::SELL, 100, 5);
    book.addOrder(2, Side::SELL, 100, 5);
    book.addOrder(3, Side::BUY, 100, 7);

    const auto& trades = book.getTrades();
    expect(trades.size() == 2, "Expected two fills");
    expect(trades[0].sellOrderId == 1, "Older order must execute first");
    expect(trades[0].quantity == 5, "First order should fill completely");
    expect(trades[1].sellOrderId == 2, "Second order should execute next");
    expect(trades[1].quantity == 2, "Second order should be partially filled");

    const auto remaining = book.getOrder(2);
    expect(remaining.has_value(), "Second sell order should remain active");
    expect(remaining->quantity == 3, "Remaining quantity should be three");
}

void testPartialFillOfRestingOrder() {
    OrderBook book(false);
    book.addOrder(1, Side::BUY, 100, 10);
    book.addOrder(2, Side::SELL, 100, 4);

    const auto remaining = book.getOrder(1);
    expect(remaining.has_value(), "Resting buy order should remain");
    expect(remaining->quantity == 6, "Resting quantity should reduce to six");
    expect(!book.hasOrder(2), "Fully filled incoming order must not rest");
}

void testIncomingOrderSweepsMultipleLevels() {
    OrderBook book(false);
    book.addOrder(1, Side::SELL, 100, 3);
    book.addOrder(2, Side::SELL, 101, 4);
    book.addOrder(3, Side::BUY, 102, 10);

    const auto& trades = book.getTrades();
    expect(trades.size() == 2, "Expected fills at two price levels");
    expect(trades[0].price == 100, "Lowest ask should execute first");
    expect(trades[1].price == 101, "Next ask should execute second");

    const auto remaining = book.getOrder(3);
    expect(remaining.has_value(), "Incoming buy remainder should rest");
    expect(remaining->quantity == 3, "Incoming remainder should equal three");
    expect(book.bestBid() == 102, "Remaining order should become best bid");
}

void testCancellationRemovesOrder() {
    OrderBook book(false);
    book.addOrder(1, Side::BUY, 100, 5);

    expect(book.cancelOrder(1), "Cancellation should succeed");
    expect(!book.hasOrder(1), "Cancelled order should be absent");
    expect(book.activeOrderCount() == 0, "Book should contain no active orders");
    expect(!book.cancelOrder(1), "Repeated cancellation should fail");
}

void testModificationLosesTimePriority() {
    OrderBook book(false);
    book.addOrder(1, Side::BUY, 100, 5);
    book.addOrder(2, Side::BUY, 100, 5);
    book.modifyOrder(1, 100, 5);
    book.addOrder(3, Side::SELL, 100, 5);

    const auto& trades = book.getTrades();
    expect(trades.size() == 1, "Expected one trade");
    expect(trades[0].buyOrderId == 2,
           "Modified order should lose its earlier FIFO priority");
}

void testDuplicateActiveOrderIdRejected() {
    OrderBook book(false);
    expect(book.addOrder(1, Side::BUY, 100, 5), "First add should succeed");
    expect(!book.addOrder(1, Side::SELL, 99, 5),
           "Duplicate active order ID should be rejected");

    const auto order = book.getOrder(1);
    expect(order.has_value(), "Original order should remain");
    expect(order->side == Side::BUY, "Original side should remain unchanged");
    expect(order->price == 100, "Original price should remain unchanged");
}

void testInvalidPriceAndQuantityRejected() {
    OrderBook book(false);
    expect(!book.addOrder(1, Side::BUY, 0, 5), "Zero price must be rejected");
    expect(!book.addOrder(2, Side::BUY, 100, 0), "Zero quantity must be rejected");
    expect(!book.addOrder(3, Side::SELL, -1, 5), "Negative price must be rejected");
    expect(!book.addOrder(4, Side::SELL, 100, -1),
           "Negative quantity must be rejected");
    expect(book.activeOrderCount() == 0, "No invalid order should enter the book");
}

void testUncrossedOrdersDoNotTrade() {
    OrderBook book(false);
    book.addOrder(1, Side::BUY, 100, 5);
    book.addOrder(2, Side::SELL, 101, 5);

    expect(book.getTrades().empty(), "Uncrossed orders must not trade");
    expect(book.bestBid() == 100, "Best bid should be 100");
    expect(book.bestAsk() == 101, "Best ask should be 101");
}

void testTradeUsesRestingOrderPrice() {
    OrderBook book(false);
    book.addOrder(1, Side::SELL, 100, 5);
    book.addOrder(2, Side::BUY, 105, 5);

    const auto& trades = book.getTrades();
    expect(trades.size() == 1, "Expected one trade");
    expect(trades[0].price == 100, "Trade price should be the resting order price");
}
}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"better price executes first", testBetterPriceExecutesFirst},
        {"FIFO within price level", testFifoWithinPriceLevel},
        {"partial fill of resting order", testPartialFillOfRestingOrder},
        {"incoming order sweeps multiple levels", testIncomingOrderSweepsMultipleLevels},
        {"cancellation removes order", testCancellationRemovesOrder},
        {"modification loses time priority", testModificationLosesTimePriority},
        {"duplicate active ID rejected", testDuplicateActiveOrderIdRejected},
        {"invalid price and quantity rejected", testInvalidPriceAndQuantityRejected},
        {"uncrossed orders do not trade", testUncrossedOrdersDoNotTrade},
        {"trade uses resting order price", testTradeUsesRestingOrderPrice},
    };

    std::size_t passed = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            ++passed;
            std::cout << "[PASS] " << name << "\n";
        } catch (const std::exception& exception) {
            std::cerr << "[FAIL] " << name << ": " << exception.what() << "\n";
        }
    }

    std::cout << "\n" << passed << "/" << tests.size() << " tests passed.\n";
    return passed == tests.size() ? 0 : 1;
}
