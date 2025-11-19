CC_CPP = clang++
CC_FLAGS_CPP = -Wall -Wextra -O2 -std=c++23
TARGET = invoke_forall_example.exe
DIR=$(shell pwd)

.PHONY: clean all

all: clean test

$(TARGET): invoke_forall_example.cpp
	$(CC_CPP) $(CC_FLAGS_CPP) $< -o $@

# TODO
# test: $(TARGET)
# 	@./$(TARGET) > fruit_picking.out
# 	@cd testy && $(CC_CPP) $(CC_FLAGS_CPP) *.cpp && ./a.out
# 	@./test.sh

clean:
	rm -f $(TARGET)
