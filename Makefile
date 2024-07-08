.PHONY: all examples format clean

all: examples

lapsa:
	git clone git@github.com:viktorsboroviks/lapsa.git
	cd lapsa; git checkout dev-v1.3

examples: \
	find_sin.o

find_sin.o: lapsa examples/find_sin.cpp
	g++ -Wall -Wextra -Werror -Wpedantic \
		-std=c++20 -O3 \
		-I./include \
		-I./lapsa/include \
		examples/find_sin.cpp -o $@

format: \
		include/tante.hpp \
		examples/find_sin.cpp
	clang-format -i $^

clean:
	rm -rf `find . -name "*.o"`
	rm -rf `find . -name "*.csv"`
	rm -rf `find . -name "*.txt"`
