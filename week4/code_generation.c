#include <stdlib.h>
#include <stdio.h>
#include "code_generation.h"

void createDFATable(dfa dfa, FILE *file){
    int state, symbol;
    printf("Numero de estados: %d \n", dfa.nstates);
    for (state = 0; state < dfa.nstates; state++){
        fprintf(file, "{");
        for (symbol = 0; symbol < 129; symbol++){
            printf("%d\n", symbol);
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

void addDFADeclaration(dfa dfa, unsigned int dfa_count, FILE *file){

}