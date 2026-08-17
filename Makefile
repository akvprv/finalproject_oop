CXX ?= g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude
BUILD_DIR := build
LIB_SOURCES := $(shell find src -name '*.cpp' | sort)
TEST_SOURCES := tests/test_main.cpp
DEMO_SOURCES := examples/demo.cpp

.PHONY: all test demo clean

all: test demo

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_SOURCES) $(TEST_SOURCES) -o $(BUILD_DIR)/proteus_tests
	$(BUILD_DIR)/proteus_tests

demo: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_SOURCES) $(DEMO_SOURCES) -o $(BUILD_DIR)/proteus_demo

clean:
	rm -rf $(BUILD_DIR)
