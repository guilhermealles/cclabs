#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
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
    mapping_size = 0;
    mapping = (intSet*) safeMalloc(sizeof(intSet) * pow(2, n.nstates));
    
    // Map the first state to the start state (0)
    intSet first_mapping = makeEmptyIntSet();
    insertIntSet(0, &first_mapping);
    mapping[mapping_size] = copyIntSet(first_mapping);
    freeIntSet(first_mapping);
    mapping_size++;
    
    dfa final_dfa = makeNFA(pow(2, n.nstates));
    int visited_count = 0;
    
    while (visited_count < mapping_size) {
        int i;
        for (i=0; i<EPSILON; i++) {
            //intSet state = epsilonStarClosureSet(movementSet(mapping[visited_count], i, n), n);
            intSet state = movementSet(epsilonStarClosureSet(mapping[visited_count], n), i, n);
            if (!isEmptyIntSet(state)) {
                // If the state is not yet mapped/expanded
                if (alreadyMapped(state) == -1) {
                    mapping[mapping_size] = copyIntSet(state);
                    mapping_size++;
                }
                
                unsigned int mapped_state = alreadyMapped(state);
                intSet new_state = makeEmptyIntSet();
                if (mapped_state == -1) {
                    fprintf(stderr, "Fatal error");
                }
                else {
                    insertIntSet(mapped_state, &new_state);
                    final_dfa.transition[visited_count][i] = copyIntSet(new_state);
                    freeIntSet(new_state);
                }
            }
        }
        visited_count++;
    }
    
    // Assign the final states
    int i;
    for (i=0; i<mapping_size; i++) {
        intSet state = copyIntSet(mapping[i]);
        intersectionIntSet(&state, n.final);
        if (!isEmptyIntSet(state)) {
            insertIntSet((unsigned int)i, &final_dfa.final);
        }
    }
    
    // Correct the number of states
    final_dfa.nstates = visited_count;
    return final_dfa;
}

intSet addToAllIntSetItems(unsigned int value, intSet set) {
    intSet set_copy = copyIntSet(set);
    intSet result = makeEmptyIntSet();
    while (!isEmptyIntSet(set_copy)) {
        int state = chooseFromIntSet(set_copy);
        deleteIntSet(state, &set_copy);
        
        state += value;
        insertIntSet(state, &result);
    }
    return result;
}

void copyTransitions(nfa sourceNfa, unsigned int sourceState, nfa *destNfa, unsigned int completedNfaStates) {
    unsigned int i;
    for (i=0; i<EPSILON+1; i++) {
        destNfa->transition[sourceState + completedNfaStates][i] = addToAllIntSetItems(completedNfaStates, sourceNfa.transition[sourceState][i]);
    }
}

void processDfaString(char *string, dfa d) {
    unsigned int string_size = (unsigned int)strlen(string);
    unsigned int string_index = 0;
    int last_accepted_index = -1;
    unsigned int current_state = d.start;
    for (string_index = 0; string_index < string_size; string_index++) {
        char c = string[string_index];
        if (isEmptyIntSet(d.transition[current_state][c])) {
            break;
        }
        else {
            current_state = chooseFromIntSet(d.transition[current_state][string[string_index]]);
            if (isMemberIntSet(current_state, d.final)) {
                last_accepted_index = string_index;
            }
        }
    }
    printf("Accepted: \"");
    if ((int)last_accepted_index > -1) {
        for (string_index = 0; string_index <= last_accepted_index; string_index++) {
            printf("%c", string[string_index]);
        }
    }
    else {
        string_index = 0;
    }
    printf("\"\nRejected: \"");
    for (; string_index < string_size; string_index++) {
        printf("%c", string[string_index]);
    }
    printf("\"\n");
}

nfa mergeNFAs(nfa *nfaArray, unsigned int nfaCount) {
    unsigned int statesCount = 0;
    int i;
    for (i=0; i<nfaCount; i++) {
        statesCount += nfaArray[i].nstates;
    }
    
    // Create a new NFA
    nfa unionNfa = makeNFA(statesCount+1);
    unionNfa.start = 0;
    unsigned int sourceNfaIndex = 0;
    unsigned int completedNfaStates = 1;
    for (i=0; i<nfaCount; i++) {
        // Will start copying a new nfa, add an epsilon transition from the starting state to the next state
        // (which is the start state of the former nfa)
        insertIntSet(completedNfaStates, &unionNfa.transition[0][EPSILON]);
        
        // Copy the individual NFA, loop through the states
        for (sourceNfaIndex=0; sourceNfaIndex < nfaArray[i].nstates; sourceNfaIndex++) {
            copyTransitions(nfaArray[i], sourceNfaIndex, &unionNfa, completedNfaStates);
        }
        intSet finalStates = addToAllIntSetItems(completedNfaStates, nfaArray[i].final);
        unionIntSet(&unionNfa.final, finalStates);
        
        completedNfaStates += nfaArray[i].nstates;
    }
    return unionNfa;
}

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
    
    nfa unionNfa = mergeNFAs(nfaArray, nfaCount);
    dfa d = convertNFAtoDFA(unionNfa);
    saveNFA("out.dfa", d);
    
    char *string = malloc(sizeof(char) * 1024);
    while (1) {
        printf("Please type a string (exit with ^c): ");
        scanf("%s", string);
        processDfaString(string, d);
        printf("\n");
    }
    
    return EXIT_SUCCESS;
}
