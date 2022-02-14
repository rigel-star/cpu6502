SRC = $(wildcard ./src/*.c)

OUT=main
WFLAGS = -Wunused-parameter -Wtautological-compare
CFLAGS=-pedantic -O0 -ggdb $(WFLAGS)

all: $(OUT)

$(OUT): $(SRC)
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm $(OUT)
