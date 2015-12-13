#include "scanner_specification.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

ScannerOptions options_section;

void initializeScannerOptions(){
    options_section.lexer_routine = "yylex";
    options_section.lexeme_name = "yytext";
    options_section.positioning_option = FALSE;
    options_section.positioning_line_name = NULL;
    options_section.positioning_column_name = NULL;
    options_section.default_action_routine = "defaultAction";
}

void setLexerRoutine(char *routine_name){
    free(options_section.lexer_routine);
    options_section.lexer_routine = malloc((strlen(routine_name) + 1) * sizeof(char));
    strcpy(options_section.lexer_routine, routine_name);
}

void setLexemeName(char *lexeme_name){
    free(options_section.lexeme_name);
    options_section.lexeme_name = malloc((strlen(lexeme_name) + 1) * sizeof(char));
    strcpy(options_section.lexeme_name, lexeme_name);
}

void setPositioningOption(int option){
    options_section.positioning_option = option;
}

void setPositioningLineName (char *name){
    free(options_section.positioning_line_name);
    options_section.positioning_line_name = malloc((strlen(name) + 1) * sizeof(char));
    strcpy(options_section.positioning_line_name, name);
}

void setPositioningColumneName (char *name){
    free(options_section.positioning_column_name);
    options_section.positioning_column_name = malloc((strlen(name) + 1) * sizeof(char));
    strcpy(options_section.positioning_column_name, name);
}

void setDefaultActionRoutineName(char *routine_name){
    free(options_section.default_action_routine);
    options_section.default_action_routine = malloc((strlen(routine_name) + 1) * sizeof(char));
    strcpy(options_section.default_action_routine, routine_name);
}

void printOptions(){
    printf("Lexer routine: %s\n", options_section.lexer_routine);
    printf("Lexeme name: %s\n", options_section.lexeme_name);
    printf("Positioning option: %d\n", options_section.positioning_option);

    if (options_section.positioning_option == TRUE){
        printf("Positioning line name: %s\n", options_section.positioning_line_name);
        printf("Positioning column name: %s\n", options_section.positioning_column_name);
    }

    printf("Default action routine: %s\n", options_section.default_action_routine);
}
