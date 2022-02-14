SRC = $(wildcard ./src/*.c)

OUT = main
MACROS = -D_POSIX_C_SOURCE=200809L # for strdup, because it is not standard C.
WFLAGS = -Wunused-parameter -Wtautological-compare
CFLAGS = -pedantic -O0 -ggdb -std=c17 $(WFLAGS) $(MACROS)

all: $(OUT)

$(OUT): $(SRC)
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm $(OUT)
