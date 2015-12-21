
#ifndef code_generator_h
#define code_generator_h

#include <stdio.h>

void addHeaders(FILE *file);
void declareGlobalVariables(FILE *file);
void declareReadDFAsFunction(FILE *file);
void declareFillTokensFunction(FILE *file);
void declareFillActionsFunction(FILE *file);
void declareLexerFunction(FILE *file);
void declareUpdateLexemeFunction(FILE *file);
void declareGetFirstNCharsFunction(FILE *file);
void declareGetNewInputFunction(FILE *file);
void declareGetGreatestFunction(FILE *file);
void declareGetSizeOfAcceptedInputFunction(FILE *file);
void declareGetNextStateFunction(FILE *file);
void createOutputCode(char* filename);

#endif /* code_generator_h */
