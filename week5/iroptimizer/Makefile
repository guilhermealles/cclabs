CC=gcc
CFLAGS=-g -O0 -Wall 
OBJECTS=misc.o quadruple.o main.o
all: scanner parser ${OBJECTS}
	${CC} -o iroptimizer ${CFLAGS} ir.tab.c ${OBJECTS} -ll -lm

scanner: ir.lex
	flex ir.lex
parser: ir.y
	bison -d -v ir.y
clean:
	rm -f ir.tab.c
	rm -f ir.tab.h
	rm -f lex.yy.c
	rm -f *.o
	rm -f *~
