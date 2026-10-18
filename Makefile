# Build and test the trading system.
#   make        build the main program (build/trading_app)
#   make test   build and run every tests/test_*.cpp
#   make run    build and run the main program
#   make clean  delete the build folder

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
BUILD    := build

LIB_SRCS  := $(wildcard src/*.cpp)
LIB_OBJS  := $(LIB_SRCS:%.cpp=$(BUILD)/%.o)
TEST_SRCS := $(wildcard tests/test_*.cpp)
TEST_BINS := $(patsubst tests/%.cpp,$(BUILD)/%,$(TEST_SRCS))

all: $(BUILD)/trading_app

# main program = main.cpp + every file in src/
$(BUILD)/trading_app: $(BUILD)/main.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# each test = its own file + every file in src/
$(BUILD)/test_%: $(BUILD)/tests/test_%.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# compile any .cpp into build/ (also writes a .d file so headers are tracked)
$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

test: $(TEST_BINS)
	@fail=0; \
	for t in $(TEST_BINS); do \
	    echo "== $$t"; \
	    ./$$t || fail=1; \
	done; \
	if [ $$fail -eq 0 ]; then echo "ALL TEST SUITES PASSED"; \
	else echo "SOME TEST SUITES FAILED"; exit 1; fi

run: all
	./$(BUILD)/trading_app

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d $(BUILD)/src/*.d $(BUILD)/tests/*.d)

.PHONY: all test run clean
.SECONDARY:
