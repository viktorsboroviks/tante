.PHONY: \
	all \
	examples \
	benchmarks \
	format \
	clang-format \
	jq-format \
	clean \
	distclean

all: examples benchmarks

iestade:
	git clone git@github.com:viktorsboroviks/iestade.git
	cd iestade; git checkout v2.1

lapsa:
	git clone git@github.com:viktorsboroviks/lapsa.git
	cd lapsa; git checkout v2.3

grafiins:
	git clone git@github.com:viktorsboroviks/grafiins.git
	cd grafiins; git checkout v5.1

rododendrs:
	git clone git@github.com:viktorsboroviks/rododendrs.git
	cd rododendrs; git checkout v1.1

garaza:
	git clone git@github.com:viktorsboroviks/garaza.git
	cd garaza; git checkout v3.1

examples: find_same.o find_sin.o

find_same.o: iestade lapsa grafiins rododendrs garaza examples/find_same.cpp
	g++ -Wall -Wextra -Werror -Wpedantic \
		-std=c++20 -O3 \
		-I./include \
		-I./iestade/include \
		-I./lapsa/include \
		-I./grafiins/include \
		-I./rododendrs/include \
		-I./garaza/include \
		examples/find_same.cpp -o $@

find_sin.o: iestade lapsa grafiins rododendrs garaza examples/find_sin.cpp
	g++ -Wall -Wextra -Werror -Wpedantic \
		-std=c++20 -O3 \
		-I./include \
		-I./iestade/include \
		-I./lapsa/include \
		-I./grafiins/include \
		-I./rododendrs/include \
		-I./garaza/include \
		examples/find_sin.cpp -o $@

benchmarks: acceptance_f.o

acceptance_f.o: benchmarks/acceptance_f.cpp
	g++ -Wall -Wextra -Werror -Wpedantic \
		-std=c++20 -O3 \
		benchmarks/acceptance_f.cpp -o $@

format: clang-format jq-format

clang-format: \
		include/tante.hpp \
		benchmarks/acceptance_f.cpp \
		examples/find_same.cpp \
		examples/find_sin.cpp
	clang-format -i $^

jq-format: \
		config.json \
		examples/find_same_config.json \
		examples/find_sin_config.json
	jq . config.json | sponge config.json
	jq . examples/find_same_config.json | sponge examples/find_same_config.json
	jq . examples/find_sin_config.json | sponge examples/find_sin_config.json

clean:
	rm -rf `find . -name "*.o"`
	rm -rf `find . -name "*.csv"`
	rm -rf `find . -name "*.txt"`

distclean: clean
	rm -rf iestade
	rm -rf lapsa
	rm -rf grafiins
	rm -rf rododendrs
	rm -rf garaza
