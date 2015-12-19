all: scannergenerator

scannergenerator: parser lexer scanner_specification.o intset.o nfa.o
	gcc -o scannergenerator parser.o lexer.o scanner_specification.o intset.o nfa.o -ll -lm

scanner_specification.o: scanner_specification.c scanner_specification.h
	gcc -c scanner_specification.c

intset.o: intset.c intset.h
	gcc -c intset.c

nfa.o: nfa.c nfa.h
	gcc -c nfa.c

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
