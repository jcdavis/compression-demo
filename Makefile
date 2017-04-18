all: setup compile

setup:
	mkdir -p build

clean:
	rm -rf build

compile: setup
	${CC} -std=c11 -Wall -O2 -o build/compression-demo *.c
