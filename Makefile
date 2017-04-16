all: setup compile

setup:
	mkdir -p build

clean:
	rm -rf build

compile: setup
	${CC} -o build/compression-demo *.c
