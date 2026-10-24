CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2

huffman: src/main.c src/huffman.c src/huffman.h
	$(CC) $(CFLAGS) -o $@ src/main.c src/huffman.c

test: huffman
	sh tests/run_tests.sh

clean:
	rm -f huffman

.PHONY: test clean
