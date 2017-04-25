CFLAGS =  -std=c11 -Wall -O2

all: setup compile

setup:
	mkdir -p build

clean:
	rm -rf build

compile: setup
	$(CC) $(CFLAGS) -o build/compression-demo *.c
