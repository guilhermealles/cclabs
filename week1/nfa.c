#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

int mapping_size;
intSet *mapping;

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

// Returns -1 in case the mapping is not found, the index of the intSet otherwise.
int alreadyMapped(intSet states) {
    int i;
    for (i=0; i<mapping_size; i++) {
        if (isEqualIntSet(states, mapping[i])) {
            return i;
        }
    }
    return -1;
}

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

intSet movementSet(intSet states, unsigned int symbol, nfa automaton) {
    intSet states_copy = copyIntSet(states);
    intSet result = makeEmptyIntSet();

    while (!isEmptyIntSet(states_copy)) {
        unsigned int state = chooseFromIntSet(states_copy);
        deleteIntSet(state, &states_copy);

        if (isEmptyIntSet(result)) {
            result = copyIntSet(movement(state, symbol, automaton));
        }

        unionIntSet(&result, movement(state, symbol, automaton));
    }


    return result;
}

dfa convertNFAtoDFA(nfa n) {
    dfa final_dfa = makeNFA(pow(2, n.nstates));
    int visited_count = 0;

    mapping[visited_count] = epsilonStarClosure(n.start, n);
    mapping_size++;


    while (visited_count <= mapping_size) {
        int i;
        for (i=0; i<EPSILON; i++) {
            intSet state = epsilonStarClosureSet(movementSet(mapping[visited_count], i, n), n);
            // If the state is not yet mapped/expanded
            if (alreadyMapped(state) == -1) {
                mapping[mapping_size] = copyIntSet(state);
                mapping_size++;
            }
            final_dfa.transition[visited_count][i] = copyIntSet(state);
        }
        visited_count++;
    }

    // Search for the starting state in the NFA
    intSet start_state = makeEmptyIntSet();
    insertIntSet(n.start, &start_state);
    int start = alreadyMapped((start_state));
    if (start == -1) {
        fprintf(stderr, "DeuBosta\n");
    }
    final_dfa.start = (unsigned int) start;

    // Assign the final states
    int i;
    for (i=0; i<mapping_size; i++) {
        intSet state = copyIntSet(mapping[i]);
        intersectionIntSet(&state, n.final);
        if (!isEmptyIntSet(state)) {
            insertIntSet((unsigned int)i, &final_dfa.final);
        }
    }

    return final_dfa;
}

int main (int argc, char **argv) {
    nfa automaton;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <nfa file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    automaton = readNFA(argv[1]);
    mapping_size = 0;
    mapping = (intSet*) safeMalloc(sizeof(intSet) * pow(2, automaton.nstates));
    dfa result = convertNFAtoDFA(automaton);
    saveNFA("out.dfa", result);
    freeNFA(automaton);
    freeNFA(result);
    return EXIT_SUCCESS;
}
