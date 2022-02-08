CFLAGS = -c -Wall $(shell pkg-config --cflags check)
TST_LIBS = $(shell pkg-config --libs check)

all:
	gcc -c -Wall -pthread 6502.c
	gcc -c -Wall 6502_test.c
	gcc -o adder 6502.o 6502_test.o -lcheck_pic -pthread -lrt -lsubunit -lm
