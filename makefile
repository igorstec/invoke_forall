CC = clang++
CC_FLAGS = -Wall -Wextra -O2 -std=c++23
TARGET = invoke_forall_example.exe
DIR=$(shell pwd)

.PHONY: clean all

all: $(TARGET) test

$(TARGET): invoke_forall_example.cpp
	$(CC) $(CC_FLAGS) $< -o $@

# TODO
test: $(TARGET)
	./$(TARGET)
	@cd tests && $(CC) $(CC_FLAGS) *.cpp && ./a.out

clean:
	rm -f $(TARGET) tests/a.out
