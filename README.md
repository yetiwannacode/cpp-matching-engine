# C++ Limit Order Book and Matching Engine

[![C++ CI](https://github.com/yetiwannacode/cpp-matching-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/yetiwannacode/cpp-matching-engine/actions/workflows/ci.yml)

A single-instrument limit-order-book and matching-engine simulator implemented in C++17.

The engine supports price-time priority, FIFO execution within price levels, partial fills, cancellation, cancel-and-replace modification, trade logging, deterministic benchmarking, automated behavioural testing, and sanitizer-backed continuous integration.

## Why I Built This

I built this project to understand the data structures and matching logic used in electronic trading systems while practising performance-conscious C++ design.

The project focuses on:

- C++17 and object-oriented design
- Data structures and algorithms
- Limit-order-book mechanics
- Price-time priority
- Efficient cancellation and modification
- Deterministic performance measurement
- Automated testing and continuous integration
- Explicit documentation of design assumptions and limitations

## Features

- Add BUY and SELL limit orders
- Match crossed orders using price-time priority
- Preserve FIFO ordering within each price level
- Support partial fills
- Match an incoming order across multiple price levels
- Cancel active resting orders
- Modify orders using cancel-and-replace semantics
- Print the current order-book state
- Maintain a history of executed trades
- Inspect the best bid and best ask
- Run deterministic single-run and multi-run benchmarks
- Validate matching behaviour through 10 automated tests
- Run AddressSanitizer and UndefinedBehaviorSanitizer checks in CI

## Supported Commands

```text
ADD <orderId> <BUY/SELL> <price> <quantity>
CANCEL <orderId>
MODIFY <orderId> <newPrice> <newQuantity>
PRINT
TRADES
BENCHMARK <numberOfOrders>
BENCHMARK_MULTI <numberOfOrders> <runs>
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
MODIFY 4 102 8
PRINT
BENCHMARK_MULTI 100000 5
EXIT
```

## Matching Rules

### Price Priority

The most competitive price is matched first:

- Higher-priced BUY orders receive priority over lower-priced BUY orders.
- Lower-priced SELL orders receive priority over higher-priced SELL orders.

### Time Priority

Orders at the same price level are executed in FIFO order.

An order inserted earlier at a price level is matched before an order inserted later at the same price.

### Trade Price

A trade executes at the price of the resting order already present in the book.

### Partial Fills

If an incoming order cannot be completely filled by one resting order, its remaining quantity continues matching against subsequent eligible orders.

An incoming order may therefore:

- Partially fill one resting order
- Fully consume one or more resting orders
- Match multiple orders at the same price
- Continue matching across multiple price levels

If quantity remains after no further eligible match is available, the remaining quantity is added to the order book.

### Order Modification

Modification follows cancel-and-replace semantics.

The original order is removed and a replacement order is inserted with the new price and quantity. The modified order therefore loses its previous FIFO priority.

## Assumptions

This project intentionally models a simplified matching engine.

The following assumptions are used:

- All submitted orders are limit orders.
- Prices and quantities are represented as integers.
- The engine manages one instrument and one order book.
- Active resting-order IDs must be unique.
- Matching occurs immediately when an incoming order crosses the opposite side.
- Orders at the same price level follow FIFO ordering.
- The execution price is the resting order's price.
- Modification is treated as cancellation followed by insertion of a new order.
- Benchmark orders are synthetically generated.
- Benchmark results represent in-memory processing, not exchange latency.

## Data Structures

The design separates price ordering, FIFO execution, and active-order lookup.

### Bid Book

```cpp
std::map<Price, OrderQueue, std::greater<Price>>
```

Bid levels are stored in descending order, making the highest bid available at the beginning of the map.

### Ask Book

```cpp
std::map<Price, OrderQueue>
```

Ask levels are stored in ascending order, making the lowest ask available at the beginning of the map.

### FIFO Order Queues

```cpp
std::list<Order>
```

Each price level stores its orders in a linked list.

New orders are appended to the back, while matching starts from the front, preserving FIFO priority.

### Active-Order Lookup

```cpp
std::unordered_map<int, OrderLocation>
```

The lookup table maps an active order ID to its location in the book.

This avoids scanning every price level during cancellation or modification.

`OrderLocation` stores information such as:

- Order side
- Price level
- Iterator to the order within its FIFO queue

### Trade Log

```cpp
std::vector<Trade>
```

Completed executions are recorded for later inspection through the `TRADES` command and automated tests.

## Matching Logic

A BUY order can match when:

```text
incoming buy price >= best available sell price
```

A SELL order can match when:

```text
incoming sell price <= best available buy price
```

At each eligible price level:

1. The oldest resting order is selected.
2. The minimum of incoming and resting quantity is executed.
3. The executed quantity is removed from both orders.
4. A fully filled resting order is removed.
5. A fully filled incoming order stops matching.
6. Any remaining incoming quantity continues to the next eligible order or price level.

## Complexity Analysis

Let:

- `P` be the number of active price levels
- `K` be the number of resting orders matched by an incoming order
- `L` be the number of affected price levels

| Operation | Expected complexity |
|---|---:|
| Locate active order by ID | `O(1)` average |
| Insert into an existing price level | `O(1)` |
| Create a new price level | `O(log P)` |
| Access best bid or ask | `O(1)` using `map.begin()` |
| Erase an order using its stored list iterator | `O(1)` |
| Cancel an order | `O(1)` average lookup + `O(log P)` price-level lookup |
| Modify an order | Cancellation + insertion |
| Match an incoming order | `O(K + L log P)` |

Actual runtime depends on workload distribution, compiler optimisation, memory allocation, and hardware.

## Project Structure

```text
cpp-matching-engine/
│
├── .github/
│   └── workflows/
│       └── ci.yml
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
│   └── OrderBookTests.cpp
│
├── sample_input.txt
├── Makefile
├── .gitignore
└── README.md
```

The generated `build/` directory is excluded from version control.

## Build Instructions

### Requirements

- C++17-compatible compiler
- GNU Make for the provided Makefile

### Build with Make

```bash
make
```

The executable is generated inside the `build` directory.

### Manual Windows PowerShell Build

```powershell
New-Item -ItemType Directory -Force build | Out-Null

g++ -Iinclude -std=c++17 -Wall -Wextra -Wpedantic `
    -Wconversion -Wshadow -O2 -DNDEBUG `
    src/main.cpp src/OrderBook.cpp `
    -o build\matching_engine.exe
```

Run using the sample command file:

```powershell
Get-Content sample_input.txt | .\build\matching_engine.exe
```

### Manual Linux Build

```bash
mkdir -p build

g++ -Iinclude \
    -std=c++17 \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Wconversion \
    -Wshadow \
    -O2 \
    -DNDEBUG \
    src/main.cpp \
    src/OrderBook.cpp \
    -o build/matching_engine
```

Run using the sample command file:

```bash
./build/matching_engine < sample_input.txt
```

## Automated Testing

Run the test suite using:

```bash
make test
```

The automated tests validate:

1. Better-price execution priority
2. FIFO execution within the same price level
3. Partial filling of a resting order
4. Matching across multiple price levels
5. Order cancellation
6. Loss of time priority after modification
7. Rejection of duplicate active order IDs
8. Rejection of invalid prices and quantities
9. Uncrossed orders remaining unexecuted
10. Execution at the resting order's price

A successful run ends with:

```text
10/10 tests passed.
```

### Manual Windows Test Build

```powershell
g++ -Iinclude -std=c++17 -Wall -Wextra -Wpedantic `
    -Wconversion -Wshadow -O0 -g3 `
    tests/OrderBookTests.cpp src/OrderBook.cpp `
    -o build\order_book_tests.exe

.\build\order_book_tests.exe
```

## Sanitizer Checks

Run the test suite with AddressSanitizer and UndefinedBehaviorSanitizer using:

```bash
make sanitize
```

The sanitizer build helps detect issues such as:

- Out-of-bounds memory access
- Use-after-free
- Invalid pointer usage
- Integer and other selected forms of undefined behaviour

The sanitizer checks run in the Ubuntu-based GitHub Actions environment.

## Continuous Integration

The repository includes a GitHub Actions workflow at:

```text
.github/workflows/ci.yml
```

For relevant pushes and pull requests, CI automatically:

1. Builds the release executable
2. Compiles and runs all behavioural tests
3. Runs the test suite with AddressSanitizer
4. Runs the test suite with UndefinedBehaviorSanitizer

## Benchmark Methodology

The built-in benchmark generates a synthetic workload and measures order submission and matching.

To improve repeatability and isolate matching-engine work:

- The complete synthetic workload is generated before timing begins.
- A fixed random seed is used.
- Repeated benchmark runs process the same order sequence.
- `std::chrono::steady_clock` measures elapsed time.
- Console logging is disabled during the timed region.
- Trade recording remains enabled.
- Best, average, and worst processing times are reported.

### Synthetic Workload

The workload contains:

- Random BUY and SELL limit orders
- Unique order IDs
- Prices ranging from 90 to 110
- Quantities ranging from 1 to 100

Run a single benchmark using:

```text
BENCHMARK 100000
```

Run repeated measurements using:

```text
BENCHMARK_MULTI 100000 5
```

From Windows PowerShell:

```powershell
"BENCHMARK_MULTI 100000 5`nEXIT" |
    .\build\matching_engine.exe
```

## Published Benchmark

### Environment

- **CPU:** 13th Gen Intel Core i5-1340P
- **Operating system:** Windows 10 Home Single Language, 64-bit
- **Windows version:** 2009
- **Compiler:** GCC 13.2.0, MSYS2 MinGW-w64
- **Language standard:** C++17
- **Optimisation flags:** `-O2 -DNDEBUG`
- **Warning flags:** `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`
- **Orders per run:** 100,000
- **Runs:** 5

### Results

| Metric | Result |
|---|---:|
| Best total processing time | 19,891 μs |
| Average total processing time | 22,857 μs |
| Worst total processing time | 26,506 μs |
| Average processing time per order | 0.2286 μs |
| Average trades generated | 79,280 |

The engine processed 100,000 synthetic orders in approximately **22.9 milliseconds on average** across five repeated runs.

The results are machine-dependent and represent a synthetic, single-process, in-memory workload. They do not include:

- Network transmission
- Exchange connectivity
- Kernel bypass
- Market-data feeds
- Persistence
- Risk checks
- Production logging
- End-to-end exchange round trips

The reported figures should therefore not be interpreted as production high-frequency-trading latency.

## Current Limitations

This implementation intentionally focuses on core order-book behaviour.

It does not currently include:

- Multiple instruments
- Market orders
- Stop or conditional orders
- Tick-size validation
- Real exchange protocols
- Network connectivity
- Persistent state or crash recovery
- Pre-trade risk checks
- Market-data publication
- Concurrent order processing
- Lock-free data structures
- Custom memory allocation
- Production-grade latency instrumentation
- Per-operation latency percentiles

## Future Improvements

- Support multiple instruments
- Add market orders and additional order types
- Add tick-size and instrument-level validation
- Add snapshot persistence and recovery
- Separate execution reports from market-data events
- Add p50, p95, and p99 per-operation latency measurements
- Evaluate object pools and custom memory allocators
- Explore cache-aware alternatives to general-purpose STL containers
- Add concurrent ingestion with explicit ordering guarantees
- Add pre-trade validation and risk controls
- Add structured output for integration and replay testing

## Disclaimer

This project is an educational simulation of selected limit-order-book and matching-engine behaviours.

It is not intended for live trading, investment decision-making, or production financial use.