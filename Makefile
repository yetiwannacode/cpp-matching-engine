CXX ?= g++
CPPFLAGS := -Iinclude
COMMON_FLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
RELEASE_FLAGS := -O2 -DNDEBUG
DEBUG_FLAGS := -O0 -g3
SANITIZER_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

BUILD_DIR := build
ENGINE := $(BUILD_DIR)/matching_engine
TEST_BIN := $(BUILD_DIR)/order_book_tests
SANITIZER_TEST_BIN := $(BUILD_DIR)/order_book_tests_sanitize

ENGINE_SOURCES := src/main.cpp src/OrderBook.cpp
TEST_SOURCES := tests/OrderBookTests.cpp src/OrderBook.cpp

.PHONY: all test sanitize clean

all: $(ENGINE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ENGINE): $(ENGINE_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(RELEASE_FLAGS) $(ENGINE_SOURCES) -o $(ENGINE)

$(TEST_BIN): $(TEST_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(DEBUG_FLAGS) $(TEST_SOURCES) -o $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(SANITIZER_TEST_BIN): $(TEST_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(DEBUG_FLAGS) $(SANITIZER_FLAGS) $(TEST_SOURCES) -o $(SANITIZER_TEST_BIN)

sanitize: $(SANITIZER_TEST_BIN)
	ASAN_OPTIONS=detect_leaks=1 ./$(SANITIZER_TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
