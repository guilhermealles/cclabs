#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nfa.h"
#include "intset.h"
#include "scanner_functions.h"


char* yytext;
unsigned int dfa_count = 6;
dfa *dfas;
int *tokens;
char *input_buffer; 
void (*actions[6]) (void); 
void readDFAs() { 
dfas = malloc(sizeof(dfa) * dfa_count);
int i;
for (i = 0; i < dfa_count; i++){
char filename[21];
sprintf(filename, "dfa%d.dfa", i);
dfas[i] = readNFA(filename);
}
} 
void noAction() { }
void fillTokens() { 
tokens = malloc(sizeof(int) * dfa_count); 
tokens[0] = -1; 
tokens[1] = -1; 
tokens[2] = -1; 
tokens[3] = -1; 
tokens[4] = -1; 
tokens[5] = -1; 
}
void fillActions() { 
actions[0] = &printFoundInput;
actions[1] = &printFoundOutput;
actions[2] = &printFoundWhile;
actions[3] = &printFoundDo;
actions[4] = &printFoundIf;
actions[5] = &printFoundThen;
}
int getNextState(int dfa_index, int state, char symbol){
intSet next_state_intset = copyIntSet(dfas[dfa_index].transition[state][(int)symbol]);
if(isEmptyIntSet(next_state_intset)){
freeIntSet(next_state_intset);
return -1;
}else{
int s = chooseFromIntSet(next_state_intset);
deleteIntSet(s, &next_state_intset);
freeIntSet(next_state_intset);
return s;
}
}
int getSizeOfAcceptedInput(int dfa_index, char* input){
int last_accepted_index = -1;
int input_size = (int)strlen(input);
int string_index;
int state = dfas[dfa_index].start;
for(string_index = 0; string_index < input_size; string_index++){
state = getNextState(dfa_index, state, input[string_index]);
if(state == -1){ // The transition doesn't exist.
break;
}
 else{
if (isMemberIntSet(state, dfas[dfa_index].final)){
last_accepted_index = string_index;
}
 }
 }
if(last_accepted_index > -1){
return last_accepted_index+1;
}else{
return 0;
}
}
int getGreatest(int array[], int size, int *greatest_index){
int i;
int greatest = -1;
for (i = 0; i < size; i++){
if (array[i] > greatest){
greatest = array[i];
*greatest_index = i;
}
}
return greatest;
}
void updateLexeme(int new_size, char* string){
yytext = realloc(yytext, sizeof(char) * (new_size + 1));
strcpy(yytext, string);
}
char* getFirstNChars(int n, char* string){
char* new_string = malloc(sizeof(char) * (n+1));
int i;
for (i = 0; i < n; i++){
new_string[i] = string[i];
}
return new_string;
}
char* getNewInput(char* input, int trimming_size){
int old_size = (int)strlen(input);
int new_size = old_size - trimming_size;
char* new_input = malloc(sizeof(char) * (new_size+1));
int i, new_index = 0;
for(i = trimming_size; i < old_size; i++){
new_input[new_index] = input[i];
new_index++;
}
free(input);
return new_input;
}
int yylex(){ 
int *accepted_sizes = malloc(sizeof(int) * dfa_count);
while(input_buffer[0] != '\0') { 
int dfa_index; 
for (dfa_index = 0; dfa_index < dfa_count; dfa_index++){
accepted_sizes[dfa_index] = getSizeOfAcceptedInput(dfa_index, input_buffer);
}
int dfa_index_accept = -1;
int accepted_size = getGreatest(accepted_sizes, dfa_count, &dfa_index_accept);
if (accepted_size == 0) {
printf("%c", input_buffer[0]);
input_buffer = getNewInput(input_buffer, 1);
}
else {
char* accepted_string = getFirstNChars(accepted_size, input_buffer);
updateLexeme(accepted_size, accepted_string);
input_buffer = getNewInput(input_buffer, accepted_size);
actions[dfa_index_accept]();
if (tokens[dfa_index_accept] != -1) { 
return tokens[dfa_index_accept];
}
}
}
return -1; 
}
int main(int argc, char **argv) {
input_buffer = malloc(1024*1024); 
readDFAs();
fillActions();
fillTokens();
while(1) { 
scanf("%s", input_buffer); 
yylex();
input_buffer[0] = '\0';
} 
} 
