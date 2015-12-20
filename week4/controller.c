#include <stdlib.h>
#include <stdio.h>
#include "intset.h"
#include "nfa.h"
#include "code_generation.h"
#include "code_generation.c"

int main (int argc, char **argv) {
    /*
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <nfa file(s)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    unsigned int nfaCount = argc-1;
    nfa *nfaArray = malloc(sizeof(nfa) * nfaCount);

    int i;
    for (i=1; i<argc; i++) {
        nfaArray[i-1] = readNFA(argv[i]);
    }
    */
    nfa* nfaArray = malloc(sizeof(nfa) * 3);
    nfaArray[0] = readNFA("/Users/Isadora/Documents/RUG/Compiler Construction/cclabs/week2/automaton1.nfa");
    nfaArray[1] = readNFA("/Users/Isadora/Documents/RUG/Compiler Construction/cclabs/week2/automaton2.nfa");
    nfaArray[2] = readNFA("/Users/Isadora/Documents/RUG/Compiler Construction/cclabs/week2/automaton3.nfa");

    dfa dfa0 = convertNFAtoDFA(nfaArray[0]);
    dfa dfa1 = convertNFAtoDFA(nfaArray[1]);
    dfa dfa2 = convertNFAtoDFA(nfaArray[2]);

    saveNFA("out0.dfa", dfa0);
    saveNFA("out1.dfa", dfa1);
    saveNFA("out2.dfa", dfa2);

    dfas[0] = dfa0;
    dfas[1] = dfa1;
    dfas[2] = dfa2;

    actions[0] = "foundWhile";
    actions[1] = "no action";
    actions[2] = "defaultAction";

    tokens[0] = "TOKEN0";
    tokens[1] = "TOKENS1";
    tokens[2] = "TOKENS2";

    FILE *file;
    file = fopen("/Users/Isadora/Documents/RUG/Compiler Construction/out.txt", "w");

    if (! file){
        fprintf(stderr, "Error: nao abriu file.\n");
    }
    else{
        printf("Abriu ok\n");
        addItem(0, file);
        addItem(1, file);
        addItem(2, file);
    }

    /*
    char *string = malloc(sizeof(char) * 1024);
    while (1) {
        printf("Please type a string (exit with ^c): ");
        scanf("%s", string);
        processDfaString(string, d);
        printf("\n");
    }
    */

    return EXIT_SUCCESS;
}
