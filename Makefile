SRC := 6502.c

OUT=main
CFLAGS=-ggdb -Wall -Wunused-parameter 

all: $(OUT)

$(OUT): $(SRC)
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm $(OUT)
