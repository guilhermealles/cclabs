
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "code_generator.h"
#include "scanner_specification.h"

void addHeaders(FILE *file) {
    fprintf(file, "#include <stdio.h>\n");
    fprintf(file, "#include <stdlib.h>\n");
    fprintf(file, "#include <string.h>\n");
    fprintf(file, "#include \"nfa.h\"\n");
    fprintf(file, "#include \"intset.h\"\n");
    fprintf(file, "#include \"scanner_functions.h\"\n");

    fprintf(file, "\n\n");
}

void declareGlobalVariables(FILE *file) {
    fprintf(file, "char* %s;\n", getOptionsSection().lexeme_name); // Declare lexeme
    fprintf(file, "unsigned int dfa_count = %d;\n", getRegexTreeCount()); // Declare dfa_count
    fprintf(file, "dfa *dfas;\n"); // Array with DFAs
    fprintf(file, "int *tokens;\n"); // Array with tokens
    fprintf(file, "char *input_buffer; \n"); // The input buffer
    fprintf(file, "void (*actions[%d]) (void); \n", getRegexTreeCount());
}

void declareReadDFAsFunction(FILE *file) {
    fprintf(file, "void readDFAs() { \n");
    fprintf(file, "dfas = malloc(sizeof(dfa) * dfa_count);\n");
    fprintf(file, "int i;\n");
    fprintf(file, "for (i = 0; i < dfa_count; i++){\n");
    fprintf(file, "char filename[21];\n");
    fprintf(file, "sprintf(filename, \"dfa%%d.dfa\", i);\n");
    fprintf(file, "dfas[i] = readNFA(filename);\n");
    fprintf(file, "}\n");
    fprintf(file, "} \n");
}

void declareFillTokensFunction(FILE *file) {
    fprintf(file, "void fillTokens() { \n");
    fprintf(file, "tokens = malloc(sizeof(int) * dfa_count); \n");
    int i;
    for (i=0; i<getRegexTreeCount(); i++) {
        if (strcmp(getRegexTokens()[i], "NO TOKEN") == 0) {
            fprintf(file, "tokens[%d] = -1; \n", i);
        }
        else {
            fprintf(file, "tokens[%d] = %s; \n", i, getRegexTokens()[i]);
        }
    }
    fprintf(file, "}\n");
}

void declareNoActionFunction(FILE *file) {
    fprintf(file, "void noAction() { }\n");
}


void declareFillActionsFunction(FILE *file) {
    fprintf(file, "void fillActions() { \n");
    int i;
    for (i=0; i<getRegexTreeCount(); i++) {
        if (strcmp(getRegexActions()[i], "NO ACTION") == 0) {
            fprintf(file, "actions[%d] = &noAction;\n", i);
        }
        else {
            fprintf(file, "actions[%d] = &%s;\n", i, getRegexActions()[i]);
        }
    }
    fprintf(file, "}\n");
}

void declareLexerFunction(FILE *file){
    fprintf(file, "int %s(){ \n", getOptionsSection().lexer_routine);
    fprintf(file, "int *accepted_sizes = malloc(sizeof(int) * dfa_count);\n");
    fprintf(file, "while(input_buffer[0] != '\\");
    fprintf(file, "0') { \n");
    fprintf(file, "int dfa_index; \n");
    fprintf(file, "for (dfa_index = 0; dfa_index < dfa_count; dfa_index++){\n");
    fprintf(file, "accepted_sizes[dfa_index] = getSizeOfAcceptedInput(dfa_index, input_buffer);\n");
    fprintf(file, "}\n");
    fprintf(file, "int dfa_index_accept = -1;\n");
    fprintf(file, "int accepted_size = getGreatest(accepted_sizes, dfa_count, &dfa_index_accept);\n");
    fprintf(file, "if (accepted_size == 0) {\n");
    fprintf(file, "printf(\"%%c\", input_buffer[0]);\n");
    fprintf(file, "input_buffer = getNewInput(input_buffer, 1);\n");
    fprintf(file, "}\n");
    fprintf(file, "else {\n");
    fprintf(file, "char* accepted_string = getFirstNChars(accepted_size, input_buffer);\n");
    fprintf(file, "updateLexeme(accepted_size, accepted_string);\n");
    fprintf(file, "input_buffer = getNewInput(input_buffer, accepted_size);\n");
    fprintf(file, "actions[dfa_index_accept]();\n"); // Call action function;
    fprintf(file, "if (tokens[dfa_index_accept] != -1) { \n");
    fprintf(file, "return tokens[dfa_index_accept];\n");
    fprintf(file, "}\n");
    fprintf(file, "}\n");
    fprintf(file, "}\n");
    fprintf(file, "return -1; \n");
    fprintf(file, "}\n");
}

void declareUpdateLexemeFunction(FILE *file){
    fprintf(file, "void updateLexeme(int new_size, char* string){\n");
    fprintf(file, "%s = realloc(%s, sizeof(char) * (new_size + 1));\n", getOptionsSection().lexeme_name, getOptionsSection().lexeme_name);
    fprintf(file, "strcpy(%s, string);\n", getOptionsSection().lexeme_name);
    fprintf(file, "}\n");
}

void declareGetFirstNCharsFunction(FILE *file){
    fprintf(file, "char* getFirstNChars(int n, char* string){\n");
    fprintf(file, "char* new_string = malloc(sizeof(char) * (n+1));\n");
    fprintf(file, "int i;\n");
    fprintf(file, "for (i = 0; i < n; i++){\n");
    fprintf(file, "new_string[i] = string[i];\n");
    fprintf(file, "}\n");
    fprintf(file, "return new_string;\n");
    fprintf(file, "}\n");
}

void declareGetNewInputFunction(FILE *file){
    fprintf(file, "char* getNewInput(char* input, int trimming_size){\n");
    fprintf(file, "int old_size = (int)strlen(input);\n");
    fprintf(file, "int new_size = old_size - trimming_size;\n");
    fprintf(file, "char* new_input = malloc(sizeof(char) * (new_size+1));\n");
    fprintf(file, "int i, new_index = 0;\n");
    fprintf(file, "for(i = trimming_size; i < old_size; i++){\n");
    fprintf(file, "new_input[new_index] = input[i];\n");
    fprintf(file, "new_index++;\n");
    fprintf(file, "}\n");
    fprintf(file, "free(input);\n");
    fprintf(file, "return new_input;\n");
    fprintf(file, "}\n");
}

void declareGetGreatestFunction(FILE *file){
    fprintf(file, "int getGreatest(int array[], int size, int *greatest_index){\n");
    fprintf(file, "int i;\n");
    fprintf(file, "int greatest = -1;\n");
    fprintf(file, "for (i = 0; i < size; i++){\n");
    fprintf(file, "if (array[i] > greatest){\n");
    fprintf(file, "greatest = array[i];\n");
    fprintf(file, "*greatest_index = i;\n");
    fprintf(file, "}\n");
    fprintf(file, "}\n");
    fprintf(file, "return greatest;\n");
    fprintf(file, "}\n");
}

void declareGetSizeOfAcceptedInputFunction(FILE *file){
    fprintf(file, "int getSizeOfAcceptedInput(int dfa_index, char* input){\n");
    fprintf(file, "int last_accepted_index = -1;\n");
    fprintf(file, "int input_size = (int)strlen(input);\n");
    fprintf(file, "int string_index;\n");
    fprintf(file, "int state = dfas[dfa_index].start;\n");
    fprintf(file, "for(string_index = 0; string_index < input_size; string_index++){\n");
    fprintf(file, "state = getNextState(dfa_index, state, input[string_index]);\n");
    fprintf(file, "if(state == -1){ // The transition doesn't exist.\n");
    fprintf(file, "break;\n");
    fprintf(file, "}\n else{\n");
    fprintf(file, "if (isMemberIntSet(state, dfas[dfa_index].final)){\n");
    fprintf(file, "last_accepted_index = string_index;\n");
    fprintf(file, "}\n }\n }\n");
    fprintf(file, "if(last_accepted_index > -1){\n");
    fprintf(file, "return last_accepted_index+1;\n");
    fprintf(file, "}else{\n");
    fprintf(file, "return 0;\n");
    fprintf(file, "}\n");
    fprintf(file, "}\n");
}

void declareGetNextStateFunction(FILE *file){
    fprintf(file, "int getNextState(int dfa_index, int state, char symbol){\n");
    fprintf(file, "intSet next_state_intset = copyIntSet(dfas[dfa_index].transition[state][(int)symbol]);\n");
    fprintf(file, "if(isEmptyIntSet(next_state_intset)){\n");
    fprintf(file, "freeIntSet(next_state_intset);\n");
    fprintf(file, "return -1;\n");
    fprintf(file, "}else{\n");
    fprintf(file, "int s = chooseFromIntSet(next_state_intset);\n");
    fprintf(file, "deleteIntSet(s, &next_state_intset);\n");
    fprintf(file, "freeIntSet(next_state_intset);\n");
    fprintf(file, "return s;\n");
    fprintf(file, "}\n");
    fprintf(file, "}\n");
}

void declareMain(FILE *file) {
    fprintf(file, "int main(int argc, char **argv) {\n");
    fprintf(file, "input_buffer = malloc(1024*1024); \n");
    fprintf(file, "readDFAs();\n");
    fprintf(file, "fillActions();\n");
    fprintf(file, "fillTokens();\n");
    fprintf(file, "while(1) { \n");
    fprintf(file, "scanf(\"%%s\", input_buffer); \n");
    fprintf(file, "%s();\n", getOptionsSection().lexer_routine);
    fprintf(file, "input_buffer[0] = '\\");
    fprintf(file, "0';\n");
    fprintf(file, "} \n");
    fprintf(file, "} \n");
}

void createOutputCode(char* filename){
    FILE *file = fopen(filename, "w");

    if(! file){
        printf("Fatal error while creating output C file.\n");
        exit(EXIT_FAILURE);
    }

    addHeaders(file);
    declareGlobalVariables(file);
    declareReadDFAsFunction(file);
    declareNoActionFunction(file);
    declareFillTokensFunction(file);
    declareFillActionsFunction(file);
    declareGetNextStateFunction(file);
    declareGetSizeOfAcceptedInputFunction(file);
    declareGetGreatestFunction(file);
    declareUpdateLexemeFunction(file);
    declareGetFirstNCharsFunction(file);
    declareGetNewInputFunction(file);
    declareLexerFunction(file);
    declareMain(file);

    fclose(file);
}
