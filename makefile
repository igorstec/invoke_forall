CC = clang++
CC_FLAGS = -Wall -Wextra -O2 -std=c++23
TARGET = invoke_forall_example.exe
DIR=$(shell pwd)

.PHONY: clean all

all: $(TARGET)

$(TARGET): invoke_forall_example.cpp
	$(CC) $(CC_FLAGS) $< -o $@

# TODO
test: $(TARGET)
	./$(TARGET)
	# @./$(TARGET) > fruit_picking.out
	# @cd testy && $(CC) $(CC_FLAGS) *.cpp && ./a.out
	# @./test.sh

clean:
	rm -f $(TARGET)
