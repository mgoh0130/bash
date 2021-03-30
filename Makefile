# NAME: Michelle Goh
#   NetId: mg2657
CC=gcc
CFLAGS= -std=c99 -pedantic -Wall -g3 -I/c/cs323/Hwk4/

all: bashLT

bashLT: process.o /c/cs323/Hwk5/mainBashLT.o /c/cs323/Hwk5/parsley.o
	${CC} ${CFLAGS} $^ -o $@

process.o: process.c /c/cs323/Hwk5/process-stub.h

clean:
		rm -f *.o bashLT
