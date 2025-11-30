CC = clang++
CC_FLAGS = -Wall -Wextra -O2 -std=c++23
TARGET = invoke_forall_example.exe
TEST_OLAF = testy.exe
DIR = $(shell pwd)

.PHONY: clean all test test_olaf

# Default target: build executable and run tests
all: $(TARGET) test

# Build main executable
$(TARGET): invoke_forall_example.cpp
	$(CC) $(CC_FLAGS) $< -o $@

# Run standard tests (always executed with 'make test')
test: $(TARGET)
	./$(TARGET)
	@cd tests && $(CC) $(CC_FLAGS) *.cpp && ./a.out

# Build and run Olaf's comprehensive tests (standalone target)
$(TEST_OLAF): testy.cpp invoke_forall.h
	$(CC) $(CC_FLAGS) $< -o $@

test_olaf: $(TEST_OLAF)
	./$(TEST_OLAF)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TEST_OLAF) tests/a.out
