#ifndef CODE_GENERATION_H
#define CODE_GENERATION_H

void createDFATable(unsigned int dfa_index, FILE *file);
void addDFADeclaration(unsigned int dfa_index, FILE *file);
void addFinalStatesDeclaration(unsigned int dfa_index, FILE* file);
void addTokenDeclaration(unsigned int dfa_index, FILE *file);
void addActionsDeclaration(unsigned int dfa_index, FILE *file);
void addItem(unsigned int dfa_index, FILE *file);


#endif
