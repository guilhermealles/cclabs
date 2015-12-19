#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "nfa.h"
#include "intset.h"
#include "scanner_specification.h"

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

// Merge 2 NFAs and add a common and unique final state.
nfa uniteNFAs(nfa nfa1, nfa nfa2){
    int i;
    nfa *nfa_array = malloc(sizeof(nfa) * 2);
    nfa_array[0] = nfa1;
    nfa_array[1] = nfa2;
    nfa merged_nfa = mergeNFAs(nfa_array, 2);
    int new_final_state = merged_nfa.nstates;

    // Add a common final state.
    nfa union_nfa = makeNFA(merged_nfa.nstates + 1);
    union_nfa.start = merged_nfa.start;

    // Copy transitions
    for (i = 0; i < merged_nfa.nstates; i++){
        copyTransitions(merged_nfa, i, &union_nfa, 0);
    }

    // Create a transition from each final state to a new final state.
    intSet final_states_copy = copyIntSet(merged_nfa.final);
    while (!isEmptyIntSet(final_states_copy)) {
        int state = chooseFromIntSet(final_states_copy);
        deleteIntSet(state, &final_states_copy);

        insertIntSet(new_final_state, &union_nfa.transition[state][EPSILON]);
    }
    insertIntSet(new_final_state, &union_nfa.final);
    return union_nfa;
}


// Concatenate NFAs nfa1 and nfa2. The start state will be the start state of nfa1 and the final state will be the final state of nfa2. The final state of nfa1 and the start state of nfa2 will be merged into one intermediate state.
nfa concatenateNFAs(nfa nfa1, nfa nfa2){
    unsigned int states_count = nfa1.nstates + nfa2.nstates - 1;
    nfa concatenated_nfa = makeNFA(states_count);

    concatenated_nfa.start = nfa1.start;
    unionIntSet(&concatenated_nfa.final, nfa2.final);

    // Copy nfa1 to the new nfa.
    int i;
    for (i = 0; i < nfa1.nstates; i++) {
        copyTransitions(nfa1, i, &concatenated_nfa, 0);
    }

    // Merge the final state of nfa1 with the initial state of nfa2.
    intSet transitionsToMerge = makeEmptyIntSet();
    int symbol;
    int last_nfa1_state = i - 1;
    for (symbol = 0; symbol < 129; symbol++){
        intSet nfa2_copy_set = copyIntSet(nfa2.transition[nfa2.start][symbol]);

        while (! isEmptyIntSet(nfa2_copy_set)) {
            int state = chooseFromIntSet(nfa2_copy_set);
            deleteIntSet(state, &nfa2_copy_set);

            insertIntSet(state + last_nfa1_state, &transitionsToMerge);
            unionIntSet(&concatenated_nfa.transition[last_nfa1_state][symbol], nfa1.transition[last_nfa1_state][symbol]);
            unionIntSet(&concatenated_nfa.transition[last_nfa1_state][symbol], transitionsToMerge);
            deleteIntSet(state + last_nfa1_state, &transitionsToMerge);
        }
    }

    // Copy the remaining transitions to the new NFA.
    int s;
    for (s = 1; s < nfa2.nstates; s++){
        for (symbol = 0; symbol < 129; symbol++) {
            if (! isEmptyIntSet(nfa2.transition[s][symbol])){
                intSet transitions = addToAllIntSetItems(last_nfa1_state, nfa2.transition[s][symbol]);
                concatenated_nfa.transition[i][symbol] = copyIntSet(transitions);
            }
        }
        i++;
    }
    concatenated_nfa.final = copyIntSet(addToAllIntSetItems(last_nfa1_state, nfa2.final));
    return concatenated_nfa;
}

// Creates the Kleene closure nfa for a given nfa, using the structure presented on "Compilers: Principles, Techniques and Tools" by Albert V. Aho et al.
nfa kleeneClosureNFA(nfa nfa){
    // Add an initial and a final state.
    struct nfa new_nfa = makeNFA(nfa.nstates + 2);
    new_nfa.start = 0;

    int new_nfa_index = nfa.start + 1;
    // Create an EPSILON transition from the new start state to the old start state.
    insertIntSet(new_nfa_index, &new_nfa.transition[0][EPSILON]);

    // Copy transitions
    int i, symbol;
    for (i = 0; i < nfa.nstates; i++){
        for (symbol = 0; symbol < 129; symbol++) {
            if (! isEmptyIntSet(nfa.transition[i][symbol])){
                intSet transitions = addToAllIntSetItems(1, nfa.transition[i][symbol]);
                new_nfa.transition[new_nfa_index][symbol] = copyIntSet(transitions);
            }
        }
        new_nfa_index++;
    }
    int old_final_state = nfa.nstates;
    int old_start_state = new_nfa.start + 1;

    // Add an epsilon transition from the old final state to the old start state.
    insertIntSet(old_start_state, &new_nfa.transition[old_final_state][EPSILON]);

    // Create an epsilon transition from the old final state to the new final state.
    insertIntSet(new_nfa_index, &new_nfa.transition[old_final_state][EPSILON]);
    intSet final_states = makeEmptyIntSet();
    insertIntSet(new_nfa_index, &final_states);
    new_nfa.final = copyIntSet(final_states);

    // Add an epsilon transition from the start state to the final state.
    insertIntSet(new_nfa_index, &new_nfa.transition[new_nfa.start][EPSILON]);

    return new_nfa;
}

// Create a new final state and add an epsilon transition from the start state to this new final state.
nfa optionalOperationNFA(nfa nfa){
    struct nfa new_nfa = makeNFA(nfa.nstates + 1);
    new_nfa.start = nfa.start;
    new_nfa.final = copyIntSet(nfa.final);

    // Copy nfa to the new nfa.
    int i;
    for (i = 0; i < nfa.nstates; i++) {
        copyTransitions(nfa, i, &new_nfa, 0);
    }

    // Add final state.
    insertIntSet(i, &new_nfa.final);

    // Add epsilon transition from start state to final state.
    insertIntSet(i, &new_nfa.transition[new_nfa.start][EPSILON]);

    return new_nfa;
}

nfa positiveClosureNFA(nfa nfa){
    // Add an initial and a final state.
    struct nfa new_nfa = makeNFA(nfa.nstates + 2);
    new_nfa.start = 0;

    int new_nfa_index = nfa.start + 1;
    // Create an EPSILON transition from the new start state to the old start state.
    insertIntSet(new_nfa_index, &new_nfa.transition[0][EPSILON]);

    // Copy transitions
    int i, symbol;
    for (i = 0; i < nfa.nstates; i++){
        for (symbol = 0; symbol < 129; symbol++) {
            if (! isEmptyIntSet(nfa.transition[i][symbol])){
                intSet transitions = addToAllIntSetItems(1, nfa.transition[i][symbol]);
                new_nfa.transition[new_nfa_index][symbol] = copyIntSet(transitions);
            }
        }
        new_nfa_index++;
    }
    int old_final_state = nfa.nstates;
    int old_start_state = new_nfa.start + 1;

    // Add an epsilon transition from the old final state to the old start state.
    insertIntSet(old_start_state, &new_nfa.transition[old_final_state][EPSILON]);

    // Create an epsilon transition from the old final state to the new final state.
    insertIntSet(new_nfa_index, &new_nfa.transition[old_final_state][EPSILON]);
    intSet final_states = makeEmptyIntSet();
    insertIntSet(new_nfa_index, &final_states);
    new_nfa.final = copyIntSet(final_states);

    return new_nfa;
}

// Given a regexp LITERAL_CHAR, LITERAL_INT or an ASCII value, creates its correspondent NFA.
nfa regexpToNFA(char* regexp){
    nfa nfa = makeNFA(2);
    nfa.start = 0;

    // Add a transition with the given symbol from the start state to the final state 1.
    if(regexp[0] == '\''){
        insertIntSet(1, &nfa.transition[0][(int)regexp[1]]);
    }
    else if(regexp[0] == '#'){
        char* symb = strtok(regexp, "#");
        int symbol = atoi(symb);
        insertIntSet(1, &nfa.transition[0][symbol]);
    }else {
        ScannerDefinition *definition;
        definition = searchDefinition(regexp);

        if (definition != NULL){
            intSet expansion_copy = copyIntSet(definition->definition_expansion);
            // Add a transition for each symbol that the definition represents.
            while (! isEmptyIntSet(expansion_copy)) {
                int symbol = chooseFromIntSet(expansion_copy);
                deleteIntSet(symbol, &expansion_copy);
                insertIntSet(1, &nfa.transition[0][symbol]);
            }
        }
        else{
            fprintf(stderr, "Error on regexpToNFA: definition not found.\n");
        }
    }
    intSet final_state = makeEmptyIntSet();
    insertIntSet(1, &final_state);
    nfa.final = copyIntSet(final_state);

    return nfa;
}
