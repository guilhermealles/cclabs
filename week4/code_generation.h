#ifndef CODE_GENERATION.H
#define CODE_GENERATION.H

void createDFATable(dfa dfa, FILE *file);
void addDFADeclaration(dfa dfa, unsigned int dfa_count, FILE *file);

#endif
