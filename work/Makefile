CC = clang

# when using gcc remove CFLAGS -Wclass-varargs and -Wunsequenced
# CC = gcc

CFLAGS=-I../ \
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

all:	z80-execute test.bin

z80-execute: z80-execute.cpp ../z80.hpp
	$(CC) $(CFLAGS) $< -o$@ -lstdc++

test.bin: test.asm
	@z80asm  -l -i $< -o $@

test: z80-execute test.bin
	@./z80-execute test.bin
