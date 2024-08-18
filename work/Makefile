CC=gcc

CFLAGS_clang=-I../ \
	-std=c++11 \
	-Wall \
	-Wfloat-equal \
	-Wshadow \
	-Wunused-variable \
	-Wsign-conversion \
	-Wclass-varargs \
	-Wtype-limits \
	-Wsequence-point \
	-Wunsequenced \
	-Werror

CFLAGS=-I../ \
	-std=c++11 \
	-Wall \
	-Wfloat-equal \
	-Wshadow \
	-Wunused-variable \
	-Wsign-conversion \
	-Wtype-limits \
	-Wsequence-point

LDFLAGS=


all:	z80-execute



z80-execute: z80-execute.o kk_ihex_read.o
	$(CC) $(LDFLAGS) -o$@ $^ -lstdc++

z80-execute.o: z80-execute.cpp ../z80.hpp
	$(CC) $(CFLAGS) -c $< -o$@

kk_ihex_read.o: kk_ihex_read.cpp kk_ihex_read.h kk_ihex.h
	$(CC) $(CFLAGS) -c $< -o$@



.PHONY: clean
clean:
	rm -rf z80-execute *.o

