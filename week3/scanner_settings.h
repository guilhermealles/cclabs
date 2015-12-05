#ifndef SCANNER_SETTINGS
#define SCANNER_SETTINGS

#define DEFAULT_LEXER "yylex";
#define DEFAULT_LEXEME "yytext";
#define DEFAULT_POSITIONING FALSE;

void parseScannerOptions(FILE *file);
void printScannerOptions();

#endif
