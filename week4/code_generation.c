#include <stdlib.h>
#include <stdio.h>
#include "code_generation.h"
#include "nfa.h"

dfa dfas[3];
char actions[3][30];
char tokens[3][30];

void createDFATable(unsigned int dfa_index, FILE *file){
    dfa dfa = dfas[dfa_index];
    int state, symbol;
    for (state = 0; state < dfa.nstates; state++){
        fprintf(file, "{");
        for (symbol = 0; symbol < 129; symbol++){
            intSet state_transition = copyIntSet(dfa.transition[state][symbol]);
            // If the transition with the current symbol doesn't exists, writes -1. Otherwise, writes the target state.
            if (isEmptyIntSet(state_transition)){
                if(symbol == 0){
                    fprintf(file, "-1");
                }
                else{
                    fprintf(file, ", -1");
                }
            }else{
                // Get the ONLY element in the intSet, since we're using a DFA
                int s = chooseFromIntSet(state_transition);
                deleteIntSet(s, &state_transition);
                fprintf(file, ", %d", s);
                // Error if more than one state was found. It's not a DFA!
                if (! isEmptyIntSet(state_transition)){
                    fprintf(stderr, "Error: more than one item on intset. Not a DFA!\n State: %d, symbol: %d", state, symbol);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if(state == dfa.nstates-1){
            fprintf(file, "}};\n");
        }else{
            fprintf(file, "},\n");
        }
    }
}

void addDFADeclaration(unsigned int dfa_index, FILE *file){
    dfa dfa = dfas[dfa_index];
    fprintf(file, "static int dfa_table%d[%d][128] = ", dfa_index, dfa.nstates);
    createDFATable(dfa_index, file);
}

void addFinalStatesDeclaration(unsigned int dfa_index, FILE* file){
    dfa dfa = dfas[dfa_index];
    intSet final_states_copy = copyIntSet(dfa.final);
    int final_states_count = 0;

    while(!isEmptyIntSet(final_states_copy)){
        int state = chooseFromIntSet(final_states_copy);
        deleteIntSet(state, &final_states_copy);
        final_states_count++;
    }

    fprintf(file, "int final_states_dfa%d[%d] = {", dfa_index, final_states_count);
    final_states_copy = copyIntSet(dfa.final);

    int i = 1;
    printf("Dfa %d: final states count: %d\n", dfa_index, final_states_count);
    while (!isEmptyIntSet(final_states_copy)) {
        int state = chooseFromIntSet(final_states_copy);
        deleteIntSet(state, &final_states_copy);
        if (i == final_states_count){
            fprintf(file, "%d};\n", state);
        }else{
            fprintf(file, "%d, ", state);
            i++;
        }

    }
}

void addStartStateDeclaration(unsigned int dfa_index, FILE *file){
    dfa dfa = dfas[dfa_index];
    fprintf(file, "int start_state_dfa%d = %d;\n", dfa_index, dfa.start);
}

void addTokenDeclaration(unsigned int dfa_index, FILE *file){
    fprintf(file, "char* token_dfa%d = \"%s\";\n", dfa_index, tokens[dfa_index]);
}

void addActionsDeclaration(unsigned int dfa_index, FILE *file){
    fprintf(file, "char* action_dfa%d = \"%s\";\n", dfa_index, actions[dfa_index]);
}

void addItem(unsigned int dfa_index, FILE *file){
    addDFADeclaration(dfa_index, file);
    addStartStateDeclaration(dfa_index, file);
    addFinalStatesDeclaration(dfa_index, file);
    addTokenDeclaration(dfa_index, file);
    addActionsDeclaration(dfa_index, file);
}
