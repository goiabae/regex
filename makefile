CC = cc
CFLAGS += -I include -Wall -Wextra -pedantic -std=c11

all: prepare build/libregex.a build/test

prepare:
	mkdir -p build

build/libregex.a: build/regex.o
	$(AR) $(ARFLAGS) $@ $^

build/regex.o: src/regex.c include/regex.h
	$(CC) $(CFLAGS) -c $< -o $@

build/test: build/main.o build/libregex.a
	$(CC) $(CFLAGS) -o $@ $^

build/main.o: src/main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r build

.PHONY: all prepare build
