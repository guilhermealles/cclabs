#include <stdlib.h>
#include <stdio.h>
#include "intset.h"
#include "nfa.h"

int main (int argc, char **argv) {
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

    //nfa unionNfa = mergeNFAs(nfaArray, nfaCount);
    //dfa d = convertNFAtoDFA(unionNfa);
    //saveNFA("out.dfa", d);

    nfa union_nfa = unionNFAs(nfaArray);
    saveNFA("out.nfa", union_nfa);
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
