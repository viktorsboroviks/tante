.PHONY: all examples format clean

all: examples

examples: \
	find_sin.o

find_sin.o: examples/find_sin.cpp
	g++ -Wall -Wextra -Werror -Wpedantic \
		-std=c++20 -O3 \
		-I./include \
		$< -o $@

format: \
		include/tante.hpp \
		examples/find_sin.cpp
	clang-format -i $^

clean:
	rm -rf `find . -name "*.o"`
	rm -rf `find . -name "*.csv"`
	rm -rf `find . -name "*.txt"`
