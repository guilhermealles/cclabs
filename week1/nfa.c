#include <stdio.h>
#include <stdlib.h>
#include "intset.h"

/* Do not change EPSILON! There are 128 ASCII characters. */
#define EPSILON 128

typedef struct nfa {
    unsigned int nstates;  /* number of states                          */
    unsigned int start;    /* number of thestart state                  */
    intSet final;          /* set of final (accepting) states           */
    intSet **transition;   /* transition: state x char -> set of states */
} nfa;

typedef nfa dfa;

static void *safeMalloc(unsigned int sz) {
    void *ptr = malloc(sz);
    if (ptr == NULL) {
        fprintf(stderr, "Fatal error: malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

nfa makeNFA(int nstates) {
    nfa n;
    unsigned int s, c;
    n.nstates = nstates;
    n.start = 0;   /* default start state */
    n.final = makeEmptyIntSet();
    n.transition = safeMalloc(nstates*sizeof(intSet *));
    for (s=0; s < nstates; s++) {
        n.transition[s] = safeMalloc(129*sizeof(intSet));
        for (c=0; c <= EPSILON; c++) {
            n.transition[s][c] = makeEmptyIntSet();
        }
    }
    return n;
}

void freeNFA(nfa n) {
    unsigned int s, c;
    freeIntSet(n.final);
    for (s=0; s < n.nstates; s++) {
        for (c=0; c <= EPSILON; c++) {
            freeIntSet(n.transition[s][c]);
        }
        free(n.transition[s]);
    }
    free(n.transition);
}

nfa readNFA(char *filename) {
    FILE *f;
    nfa n;
    unsigned int state, nstates;

    f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Fatal error: failed to open file\n");
        exit(EXIT_FAILURE);
    }
    fscanf(f, "%u\n", &nstates);
    n = makeNFA(nstates);
    fscanf(f, "%u\n", &n.start);
    n.final = readIntSetFromFile(f);
    /* read transitions */
    while (fscanf(f, "%u\n", &state) == 1) {
        int c;
        do {
            c = getc(f);
        } while ((c == ' ') || (c == '\t') || (c == '\n'));
        /* c can be a quote ('), 'e' (from eps), or '#' (ascii number) */
        switch (c) {
            case '\'':
                c = fgetc(f);
                fgetc(f);  /* skip closing quote */
                break;
            case 'e':
                fgetc(f);  /* skip 'p' */
                fgetc(f);  /* skip 's' */
                c = EPSILON;
                break;
            case '#':
                fscanf(f, "%d", &c);
                break;
            default:
                fprintf(stderr, "Syntax error in automata file   %c\n", c);
                exit(EXIT_FAILURE);
        }
        n.transition[state][c] = readIntSetFromFile(f);
    }
    fclose(f);
    return n;
}

void saveNFA(char *filename, nfa n) {
    FILE *f;
    unsigned int c, state;

    f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Fatal error: failed to open file\n");
        exit(EXIT_FAILURE);
    }
    fprintf(f, "%d\n%d\n", n.nstates, n.start);
    fprintlnIntSet(f, n.final);
    for (state = 0; state < n.nstates; state++) {
        for (c=0; c<= EPSILON; c++) {
            if (!isEmptyIntSet(n.transition[state][c])) {
                fprintf(f, "%d ", state);
                if (c == EPSILON) {
                    fprintf(f, "eps ");
                } else {
                    if (c > ' ') {
                        fprintf(f, "'%c' ", c);
                    } else {
                        fprintf(f, "#%d ", c);
                    }
                }
                fprintlnIntSet(f, n.transition[state][c]);
            }
        }
    }
    fclose(f);
}
/*
dfa convertNFAtoDFA(nfa n) {
    intSet visited_states = makeEmptyIntSet();

    int dfa_states_count = pow(2, n.nstates);
    dfa d = makeNFA(dfa_states_count);
}
*/

intSet epsilonClosure(int state, nfa n) {
    intSet closure = copyIntSet(n.transition[state][EPSILON]);
    return closure;
}

intSet epsilonStarClosure(int state, nfa n) {
    intSet current_state_closure = epsilonClosure(state, n);
    intSet current_state_closure_copy = copyIntSet(current_state_closure);

    while (!isEmptyIntSet(current_state_closure_copy)) {
        unsigned int state = chooseFromIntSet(current_state_closure_copy);
        deleteIntSet(state, &current_state_closure_copy);

        unionIntSet(&current_state_closure, epsilonStarClosure(state, n));
    }

    // Add the state itself in the intSet.
    insertIntSet(state, &current_state_closure);

    return current_state_closure;
}

intSet epsilonStarClosureSet(intSet states, nfa n) {
    intSet states_copy = copyIntSet(states);
    intSet final_closure = makeEmptyIntSet();

    while (!isEmptyIntSet(states_copy)) {
        unsigned int state = chooseFromIntSet(states_copy);
        deleteIntSet(state, &states_copy);

        if (isEmptyIntSet(final_closure)) {
            final_closure = copyIntSet(epsilonStarClosure(state,n));
        }

        unionIntSet(&final_closure, epsilonStarClosure(state, n));
    }

    return final_closure;
}

intSet movement(unsigned int state, unsigned int symbol, nfa automaton) {
    intSet result = copyIntSet(automaton.transition[state][symbol]);

    return result;
}

int main (int argc, char **argv) {
    nfa automaton;
    /*
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <nfa file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    automaton = readNFA(argv[1]);
     */
    automaton = readNFA("/Users/Alles/Desktop/tmp/tmp/example.nfa");
    //intSet demo = makeEmptyIntSet();
    //insertIntSet(0, &demo);
    //insertIntSet(1, &demo);
    //insertIntSet(2, &demo);
    //insertIntSet(3, &demo);
    //printIntSet(demo);
    printIntSet(epsilonStarClosure(3, automaton));
    printf("\n");
    //saveNFA("out.nfa", automaton);
    freeNFA(automaton);
    return EXIT_SUCCESS;
}
