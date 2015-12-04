#ifndef SCANNER_SETTINGS
#define SCANNER_SETTINGS

#define TRUE 1
#define FALSE 0

#define DEFAULT_LEXER "yylex";
#define DEFAULT_LEXEME "yytext";
#define DEFAULT_POSITIONING FALSE;

void parseScannerOptions(FILE *file);
void printScannerSettings();
void getNextSymbol(FILE *f, char *buffer, char delimiter, int error_on_whitespace);
inline int isValidChar(char c);

#endif
