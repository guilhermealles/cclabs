all: scannergenerator

scannergenerator: parser lexer scanner_specification.o
	gcc -o scannergenerator parser.o scanner_specification.o lexer.o -ll -lm

scanner_specification.o: scanner_specification.c scanner_specification.h
	gcc -c scanner_specification.c

parser: grammar.g
	cp grammar.g parser.g
	../LLnextgen-0.5.5/LLnextgen parser.g
	gcc -c parser.c
	rm -f parser.g

lexer: specification.lex
	flex specification.lex
	mv lex.yy.c lexer.c
	gcc -c lexer.c

clean:
	rm -f *~
	rm -f *.o
	rm -f parser.c
	rm -f parser.h
	rm -f lexer.c
