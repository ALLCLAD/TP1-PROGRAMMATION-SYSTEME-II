# Makefile du TP1 (CSC 374)
CC     = gcc
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -O2 -g

all: csim tracegen

csim: csim.c cachelab.c cachelab.h
	$(CC) $(CFLAGS) -o csim csim.c cachelab.c

# La partie B se compile sans optimisation, comme dans test-trans.py
tracegen: tracegen.c trans.c cachelab.c cachelab.h
	$(CC) -std=c17 -Wall -Wextra -O0 -g -o tracegen tracegen.c trans.c cachelab.c

test: csim
	python3 test-csim.py
	python3 test-trans.py

fuites: csim
	valgrind --leak-check=full --error-exitcode=1 ./csim -s 4 -E 2 -b 4 -t traces/histo.trace

remise:
	tar czf ../$$(whoami)_tp1.tar.gz csim.c trans.c rapport.pdf

clean:
	rm -f csim tracegen .csim_resultats trans_*.trace

.PHONY: all test fuites remise clean
