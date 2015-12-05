#ifndef SCANNER_LIB_H
#define SCANNER_LIB_H

#define TRUE 1
#define FALSE 0

void getNextSymbol(FILE *f, char *buffer, char delimiter, int error_on_whitespace);
int isValidChar(char c);

#endif
