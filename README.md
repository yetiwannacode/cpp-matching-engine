# C++ Matching Engine Simulator

A simplified matching engine and limit order book simulator built in C++.

The project implements price-time priority matching, partial fills, order cancellation, order modification, trade logging, and synthetic order-flow benchmarking.

## Why I Built This

I built this project to understand the core data structures and matching logic used in electronic trading systems, and to practice performance-conscious C++ design.

The focus of this project is on:

- C++ and object-oriented design
- Data structures and algorithms
- Order book mechanics
- Price-time priority matching
- Efficient order lookup for cancel/modify operations
- Basic latency benchmarking using synthetic order flow

## Features

- Add buy and sell limit orders
- Match orders using price-time priority
- Support partial fills
- Cancel existing orders
- Modify existing orders
- Print current order book state
- Maintain executed trade logs
- Run synthetic order-flow benchmarks using `std::chrono`

## Project Outcome

The simulator currently supports the complete flow of a simplified limit order book:

- Adding buy and sell limit orders
- Matching crossed orders using price-time priority
- Handling partial fills
- Cancelling resting orders
- Modifying existing orders as cancel-and-replace
- Printing the current order book
- Logging executed trades
- Running synthetic order-flow benchmarks

On a sample benchmark of 100,000 synthetic orders, the engine processed the orders in 68,145 microseconds, with an average processing time of 0.6815 microseconds per order on my system.

These benchmark numbers are machine-dependent and are intended only to measure this simplified in-memory implementation. They should not be interpreted as production-level HFT latency.

## Supported Commands

```text
ADD <orderId> <BUY/SELL> <price> <quantity>
CANCEL <orderId>
MODIFY <orderId> <newPrice> <newQuantity>
PRINT
TRADES
BENCHMARK <numberOfOrders>
EXIT
```

## Example Input

```text
ADD 1 BUY 100 10
ADD 2 SELL 99 5
ADD 3 SELL 101 7
ADD 4 BUY 101 10
PRINT
TRADES
CANCEL 1
PRINT
MODIFY 4 102 8
PRINT
TRADES
BENCHMARK 100000
EXIT
```

## Example Output

```text
C++ Matching Engine Simulator
Supported commands:
ADD <orderId> <BUY/SELL> <price> <quantity>
CANCEL <orderId>
MODIFY <orderId> <newPrice> <newQuantity>
PRINT
TRADES
BENCHMARK <numberOfOrders>
EXIT

TRADE: BuyOrder=1 SellOrder=2 Price=100 Quantity=5
TRADE: BuyOrder=4 SellOrder=3 Price=101 Quantity=7

===== ORDER BOOK =====
SELL ORDERS:

BUY ORDERS:
Price 101: [ID=4, Qty=3]
Price 100: [ID=1, Qty=5]
======================

===== TRADES =====
BuyOrder=1 SellOrder=2 Price=100 Quantity=5
BuyOrder=4 SellOrder=3 Price=101 Quantity=7
==================

Cancelled order: 1

===== ORDER BOOK =====
SELL ORDERS:

BUY ORDERS:
Price 101: [ID=4, Qty=3]
======================

Cancelled order: 4
Modified order: 4 NewPrice=102 NewQuantity=8

===== ORDER BOOK =====
SELL ORDERS:

BUY ORDERS:
Price 102: [ID=4, Qty=8]
======================

===== TRADES =====
BuyOrder=1 SellOrder=2 Price=100 Quantity=5
BuyOrder=4 SellOrder=3 Price=101 Quantity=7
==================

===== BENCHMARK =====
Processed orders: 100000
Total time: 68145 microseconds
Average processing time: 0.6815 microseconds/order
Trades generated: 79280
=====================
```

Benchmark results are machine-dependent and may vary across systems.

## Assumptions and Design Choices

This project intentionally models a simplified single-instrument matching engine.

The following assumptions are used:

- All orders are limit orders.
- Prices and quantities are represented as integers.
- The engine handles one instrument/order book at a time.
- Order IDs are assumed to be unique.
- Matching happens immediately when an incoming order crosses the opposite side of the book.
- Within the same price level, orders follow FIFO ordering to preserve time priority.
- The trade price is taken from the resting order already present in the book.
- A modified order is treated as a cancel followed by a new add, so it receives a new timestamp and loses its earlier time priority.
- Benchmarking uses synthetic randomly generated orders and does not represent real exchange latency.

This project does not attempt to model real-world trading infrastructure completely. It focuses on the core matching logic and data-structure design.

## Data Structures Used

### Buy Book

```cpp
std::map<Price, OrderQueue, std::greater<Price>>
```

The buy book is sorted in descending price order, so the highest bid is always available first.

### Sell Book

```cpp
std::map<Price, OrderQueue>
```

The sell book is sorted in ascending price order, so the lowest ask is always available first.

### Order Queues

```cpp
std::list<Order>
```

Each price level stores orders in FIFO order to preserve time priority.

### Order Lookup

```cpp
std::unordered_map<int, OrderLocation>
```

This allows efficient lookup of orders for cancellation and modification.

The `OrderLocation` stores the order side, price level, and iterator to the order inside the corresponding price-level queue.

## Matching Logic

Buy orders match against the lowest available sell price when:

```text
buy price >= best sell price
```

Sell orders match against the highest available buy price when:

```text
sell price <= best buy price
```

Within the same price level, older orders are matched first. This preserves price-time priority.

If an incoming order is only partially filled, its remaining quantity continues matching until no more eligible opposite-side orders exist. If some quantity still remains after matching, it is added to the book.

## Complexity Analysis

Let `P` be the number of active price levels and `K` be the number of orders or price levels matched by an incoming order.

| Operation | Complexity |
|---|---|
| Add order to book | `O(log P)` |
| Access best bid/ask | `O(1)` using `map.begin()` |
| Match order | `O(K log P)` in the worst case due to price-level removals |
| Cancel order | `O(1)` lookup + `O(1)` erase using stored list iterator |
| Modify order | Cancel + Add |

## Benchmarking

The benchmark command generates synthetic buy and sell orders with randomized prices and quantities.

Example:

```text
BENCHMARK 100000
```

The benchmark reports:

- Number of processed orders
- Total processing time
- Average processing time per order
- Number of trades generated

This benchmark is useful for comparing changes within this project, but it should not be interpreted as production-level HFT latency. It does not include network delays, exchange connectivity, kernel bypass, risk checks, persistence, logging overhead, or real market-data feed handling.

## How to Build

### Windows PowerShell

Compile:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src/main.cpp src/OrderBook.cpp -o matching_engine.exe
```

Run with sample input:

```powershell
Get-Content sample_input.txt | .\matching_engine.exe
```

### Linux / Git Bash

Compile:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src/main.cpp src/OrderBook.cpp -o matching_engine
```

Run with sample input:

```bash
./matching_engine < sample_input.txt
```

## Project Structure

```text
cpp-matching-engine/
│
├── include/
│   ├── Order.hpp
│   ├── Trade.hpp
│   └── OrderBook.hpp
│
├── src/
│   ├── OrderBook.cpp
│   └── main.cpp
│
├── tests/
│   └── sample_test.txt
│
├── sample_input.txt
├── Makefile
├── .gitignore
└── README.md
```

## Current Limitations

This is a simplified educational matching engine. It does not include:

- Multiple instruments
- Market orders
- Stop orders
- Tick-size validation
- Real exchange connectivity
- Network feed handling
- Risk checks
- Persistent storage
- Multithreaded market data ingestion
- Production-grade latency measurement
- p50, p95, or p99 latency distribution tracking

## Future Improvements

- Add socket-based order input
- Add p50, p95, and p99 latency tracking
- Add object pooling to reduce dynamic allocations
- Replace STL containers with more cache-friendly structures
- Add multithreaded market data simulation
- Add unit tests for matching, cancellation, modification, and edge cases
- Support multiple instruments using separate order books
- Add stricter command validation and error handling