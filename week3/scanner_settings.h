#ifndef SCANNER_SETTINGS
#define SCANNER_SETTINGS

#define TRUE 1
#define FALSE 0

#define DEFAULT_LEXER "yylex";
#define DEFAULT_LEXEME "yytext";
#define DEFAULT_POSITIONING FALSE;

char *lexer = DEFAULT_LEXER;

char *lexeme = DEFAULT_LEXEME;

int positioning = FALSE;
char *positioning_line_var;
char *positioning_column_var;

int exists_default_action = FALSE;
char *default_action;

void parseScannerOptions(FILE *file);
#endif
