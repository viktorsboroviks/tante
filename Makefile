.PHONY: all examples benchmarks format clean distclean

all: examples benchmarks

lapsa:
	git clone git@github.com:viktorsboroviks/lapsa.git
	cd lapsa; git checkout v2.2

grafiins:
	git clone git@github.com:viktorsboroviks/grafiins.git
	cd grafiins; git checkout v3.6

rododendrs:
	git clone git@github.com:viktorsboroviks/rododendrs.git
	cd rododendrs; git checkout v1.1

garaza:
	git clone git@github.com:viktorsboroviks/garaza.git
	cd garaza; git checkout v2.0

examples: find_same.o
#find_sin.o

#find_sin.o: lapsa grafiins examples/find_sin.cpp
#	g++ -Wall -Wextra -Werror -Wpedantic \
#		-std=c++20 -O3 \
#		-I./include \
#		-I./lapsa/include \
#		-I./grafiins/include \
#		examples/find_sin.cpp -o $@

find_same.o: lapsa grafiins rododendrs garaza examples/find_same.cpp
	g++ -Wall -Wextra -Werror -Wpedantic \
		-std=c++20 -O3 \
		-I./include \
		-I./lapsa/include \
		-I./grafiins/include \
		-I./rododendrs/include \
		-I./garaza/include \
		examples/find_same.cpp -o $@

benchmarks: acceptance_f.o

acceptance_f.o: benchmarks/acceptance_f.cpp
	g++ -Wall -Wextra -Werror -Wpedantic \
		-std=c++20 -O3 \
		benchmarks/acceptance_f.cpp -o $@

format: \
		include/tante.hpp \
		examples/find_same.cpp \
		examples/find_sin.cpp \
		benchmarks/acceptance_f.cpp
	clang-format -i $^

clean:
	rm -rf `find . -name "*.o"`
	rm -rf `find . -name "*.csv"`
	rm -rf `find . -name "*.txt"`

distclean: clean
	rm -rf lapsa
	rm -rf grafiins
	rm -rf rododendrs
	rm -rf garaza
