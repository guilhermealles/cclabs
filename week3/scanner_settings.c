#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner_settings.h"

#define BUFFER_SIZE 512

char *lexer = DEFAULT_LEXER;
char *lexeme = DEFAULT_LEXEME;

int positioning_set = FALSE;
char *positioning_line_var;
char *positioning_column_var;

int exists_default_action = FALSE;
char *default_action;


void parseScannerOptions(FILE *file) {
    char *buffer = malloc(sizeof(char) * BUFFER_SIZE);

    int end_of_section = FALSE;
    while (!end_of_section) {
        fscanf(file, "%s", buffer);
        if (strcmp(buffer, "lexer") == 0) {

            fscanf(file, "%s", buffer);
            int lexer_string_size = strlen(buffer);
            if (buffer[lexer_string_size-1] == ';') {
                // Remove ';' from the string
                buffer[lexer_string_size-1] = '\0';
                lexer_string_size--;
                lexer = malloc(sizeof(char) * lexer_string_size);
                strcpy(lexer, buffer);
            }
            else {
                fprintf(stderr, "Error: expected \';\' after %s.\n", buffer);
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(buffer, "lexeme") == 0) {
            fscanf(file, "%s", buffer);
            int lexeme_string_size = strlen(buffer);
            if (buffer[lexeme_string_size-1] == ';') {
                // Remove ';' from the string
                buffer[lexeme_string_size-1] = '\0';
                lexeme_string_size--;
                lexeme = malloc(sizeof(char) * lexeme_string_size);
                strcpy(lexeme, buffer);
            }
            else {
                fprintf(stderr, "Error: expected \';\' after %s\n", buffer);
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(buffer, "positioning") == 0) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "on;") == 0) {
                fscanf(file, "%s", buffer);
                if (strcmp(buffer, "where") == 0) {
                    fscanf(file, "%s", buffer);
                    if (strcmp(buffer, "line") == 0) {
                        positioning_set = TRUE;
                        fscanf(file, "%s", buffer);
                        int line_string_size = strlen(buffer);
                        if (buffer[line_string_size-1] == ';') {
                            // Remove ';' from the string
                            buffer[line_string_size-1] = '\0';
                            line_string_size--;
                            positioning_line_var = malloc(sizeof(char) * line_string_size);
                            strcpy(positioning_line_var, buffer);
                        }
                        else {
                            fprintf(stderr, "Error: expected \';\' after %s\n", buffer);
                            exit(EXIT_FAILURE);
                        }

                        fscanf(file, "%s", buffer);
                        if (strcmp(buffer, "column") == 0) {
                            fscanf(file, "%s", buffer);
                            int column_string_size = strlen(buffer);
                            if (buffer[column_string_size-1] == ';') {
                                // Remove ';' from the string
                                buffer[column_string_size-1] = '\0';
                                column_string_size--;
                                positioning_column_var = malloc(sizeof(char) * column_string_size);
                                strcpy(positioning_column_var, buffer);
                            }
                            else {
                                fprintf(stderr, "Error: expected \';\' after %s\n", buffer);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else {
                            fprintf(stderr, "Error: expected \"column\".\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else {
                        fprintf(stderr, "Error: expected \"line\".\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    fprintf(stderr, "Error: expected \"where\".\n");
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(buffer, "off;") == 0) {
                // no action
            }
            else {
                fprintf(stderr, "Error: expected \"on;\" or \"off;\".\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(buffer, "default") == 0) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "action") == 0) {
                exists_default_action = TRUE;

                fscanf(file, "%s", buffer);
                int default_action_string_size = strlen(buffer);
                if (buffer[default_action_string_size-1] == ';') {
                    // Remove ';' from the string
                    buffer[default_action_string_size-1] = '\0';
                    default_action_string_size--;
                    default_action = malloc(sizeof(char) * default_action_string_size);
                    strcpy(default_action, buffer);
                }
                else {
                    fprintf(stderr, "Error: expected \';\' after %s\n", buffer);
                    exit(EXIT_FAILURE);
                }
            }
            else {
                fprintf(stderr, "Error: expected \"action\".\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(buffer, "end") == 0) {
            end_of_section = TRUE;
        }
        else {
            fprintf(stderr, "Error: unexpected keyword, found \"%s\".\n", buffer);
            exit(EXIT_FAILURE);
        }
    }
    // Now expecting: "section options;"
    fscanf(file, "%s", buffer);
    if (strcmp(buffer, "section") == 0) {
        fscanf(file, "%s", buffer);
        if (strcmp(buffer, "options;") == 0) {
            // no action
        }
        else {
            fprintf(stderr, "Error: expected \"options;\", found %s", buffer);
            exit(EXIT_FAILURE);
        }
    }
    else {
        fprintf(stderr, "Error: expected \"section\", found %s", buffer);
        exit(EXIT_FAILURE);
    }
}

void printScannerSettings() {
    printf("Lexer option: \"%s\".\n", lexer);
    printf("Lexeme option: \"%s\".\n", lexeme);
    printf("Positioning option: \"%d\".\n", positioning_set);
    if (positioning_set == TRUE) {
        printf("Positioning line variable: \"%s\".\n", positioning_line_var);
        printf("Positioning column variable: \"%s\".\n", positioning_column_var);
    }
    printf("Default action exists: \"%d\".\n", exists_default_action);
    if (exists_default_action) {
        printf("Default action function name: \"%s\".\n", default_action);
    }
}

void getNextSymbol(FILE *f, char *buffer, char delimiter, int error_on_whitespace) {
    unsigned int buffer_index = 0;
    char c = fgetc(f);
    while (c == ' ') {
        c = fgetc(f);
    }
    if (c != delimiter) {
        buffer[buffer_index] = c;
        buffer_index++;
    }
    while (c != delimiter) {
        c = fgetc(f);
        if (c == ' ') {
            c = fgetc(f);
            while (c == ' ') {
                c = fgetc(f);
            }
            if (c != delimiter) {
                if (error_on_whitespace) {
                    fprintf(stderr, "Error: expected \"%c\".\n", delimiter);
                    exit(EXIT_FAILURE);
                }
                else {
                    buffer[buffer_index] = c;
                    buffer_index++;
                }
            }
        }
        else if (c == '\n') {
            fprintf(stderr, "Error: unexpected line break.\n");
            exit(EXIT_FAILURE);
        }
        else if (c == '\0') {
            fprintf(stderr, "Error: unexpected end-of-string.\n");
            exit(EXIT_FAILURE);
        }
        else if (c != delimiter) {
            buffer[buffer_index] = c;
            buffer_index++;
        }
    }
    buffer[buffer_index] = '\0';
    buffer_index++;
}

// returns true if c is 0~9, a~z, A~Z or _
inline int isValidChar(char c) {
    if (c < 48) {
        return FALSE;
    }
    if (c > 57 && c < 65) {
        return FALSE;
    }
    if (c > 90 && c < 97 && c != 95) {
        return FALSE;
    }
    if (c > 122) {
        return FALSE;
    }
    return TRUE;
}
