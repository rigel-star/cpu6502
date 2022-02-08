SRC = $(wildcard ./src/*.c)

OUT=main
CFLAGS=-pedantic -O0 -ggdb -Wall -Wunused-parameter 

all: $(OUT)

$(OUT): $(SRC)
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm $(OUT)
