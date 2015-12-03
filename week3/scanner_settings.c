#include <stdio.h>
#include <string.h>
#include "scanner_settings.h"

#define BUFFER_SIZE 512

// FALTA VER O ;;;;;;;
void parseScannerOptions(FILE *file) {
    char *buffer = malloc(sizeof(char) * BUFFER_SIZE);

    int end_of_section = FALSE;
    while (!end_of_section) {
        fscanf(file, "%s", buffer);
        if (strcmp(buffer, "lexer") == 0) {
            fscanf(file, "%s", buffer);
            int lexer_string_size = strlen(buffer) + 1;
            free(lexer);
            lexer = malloc(sizeof(char) * lexer_string_size);
            strcpy(lexer, buffer);
        }
        else if (strcmp(buffer, "lexeme") == 0) {
            fscanf(file, "%s", buffer);
            int lexeme_string_size = strlen(buffer) + 1;
            free(lexeme);
            lexeme = malloc(sizeof(char) * lexeme_string_size);
            strcpy(lexeme, buffer);
        }
        else if (strcmp(buffer, "positioning") == 0) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "on") == 0) {
                fscanf(file, "%s", buffer);
                if (strcmp(buffer, "where") == 0) {
                    fscanf(file, "%s", buffer);
                    if (strcmp(buffer, "line") == 0) {
                        fscanf(file, "%s", buffer);
                        int line_string_size = strlen(buffer) + 1;
                        free (positioning_line_var);
                        positioning_line_var = malloc(sizeof(char) * line_string_size);
                        strcpy(positioning_line_var, buffer);

                        fscanf(file, "%s", buffer);
                        if (strcmp(buffer, "column") == 0) {
                            fscanf(file, "%s", buffer);
                            int column_string_size = strlen(buffer) + 1;
                            free (positioning_column_var);
                            positioning_column_var = malloc(sizeof(char) * column_string_size);
                            strcpy(positioning_column_var, buffer);
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
            else if (strcmp(buffer, "off") == 0) {
                // no action
            }
            else {
                fprintf(stderr, "Error: expected \"on\" or \"off\".\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(buffer, "default") == 0) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "action") == 0) {
                exists_default_action = TRUE;

                fscanf(file, "%s", buffer);
                int default_action_string_size = strlen(buffer) + 1;
                free (default_action);
                default_action = malloc(sizeof(char) * default_action_string_size);
                strcpy(default_action, buffer);
            }
            else {
                fprintf(stderr, "Error: expected \"action\".\n");
                exit(EXIT_FAILURE);
            }
        }
        if (strcmp(buffer, "end") == 0) {
            end_of_section = TRUE;
        }
        else {
            fprintf(stderr, "Parsing error: found %s\n", buffer);
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
