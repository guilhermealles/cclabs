#ifndef SCANNER_SPECIFICATION
#define SCANNER_SPECIFICATION

#define TRUE 1
#define FALSE 0

typedef struct ScannerOptions{
    char *lexer_routine;
    char *lexeme_name;
    int positioning_option;
    char *positioning_line_name;
    char *positioning_column_name;
    char *default_action_routine;
}ScannerOptions;

void initializeScannerOptions();
void setLexerRoutine(char *routine_name);
void setLexemeName(char *lexeme_name);
void setPositioningOption(int option);
void setPositioningLineName (char *name);
void setPositioningColumneName (char *name);
void setDefaultActionRoutineName(char *routine_name);
void printOptions();

#endif
