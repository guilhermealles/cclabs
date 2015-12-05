#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/intset.h"
#include "scanner_definitions.h"
#include "lib/scanner_lib.h"

#define MAX_IDENTIFIER_SIZE 129 // maximum string length for a definition name
#define BUFFER_SIZE 1024

unsigned int definitions_count = 0;
char **definition_names = NULL;
intSet *definition_equivalences = NULL;

void parseScannerDefinitions(FILE *file) {
    initializeScannerDefinitions();
    char *buffer = malloc(sizeof(char) * BUFFER_SIZE);

    int end_of_section = FALSE;
    while (!end_of_section) {
        fscanf(file, "%s", buffer);
        if (strcmp(buffer, "define") == 0) {
            getNextSymbol(file, buffer, '=', TRUE);
            unsigned int identifier_size = strlen(buffer) + 1;
            if (identifier_size < 129) {
                // Copy the definition name to the definition_names table
                definition_names = realloc(definition_names, sizeof(char*) * (definitions_count+1));
                definition_names[definitions_count] = malloc(sizeof(char) * identifier_size);
                strcpy(definition_names[definitions_count], buffer);

                getNextSymbol(file, buffer, ';', FALSE);
                intSet equivalence = parseDefinition(buffer, (unsigned int)strlen(buffer));
                definition_equivalences = realloc(definition_equivalences, sizeof(intSet) * (definitions_count+1));
                definition_equivalences[definitions_count] = equivalence;

                definitions_count++;
            }
            else {
                fprintf(stderr, "Error: identifier in \"define\" section: definition is too big.");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(buffer, "end") == 0) {
            end_of_section = TRUE;
        }
        else {
            fprintf(stderr, "Error: expected \"define\" or \"end section\" keywords before \"%s\".\n", buffer);
            exit(EXIT_FAILURE);
        }
    }
    // Now expecting: "section defines;"
    fscanf(file, "%s", buffer);
    if (strcmp(buffer, "section") == 0) {
        fscanf(file, "%s", buffer);
        if (strcmp(buffer, "defines;") == 0) {
            // no action
        }
        else {
            fprintf(stderr, "Error: expected \"defines;\", found %s", buffer);
            exit(EXIT_FAILURE);
        }
    }
    else {
        fprintf(stderr, "Error: expected \"section\", found %s", buffer);
        exit(EXIT_FAILURE);
    }
}

void initializeScannerDefinitions() {
    definition_names = malloc(sizeof(char*) * (definitions_count+1));
    definition_equivalences = malloc(sizeof(intSet*) * (definitions_count+1));
}

intSet parseDefinition(char *buffer, unsigned int buffer_size) {
    unsigned int buffer_index = 0;
    if (buffer[buffer_index] != '[') {
        fprintf(stderr, "Error: expected \"[\" in definition.\n");
        exit(EXIT_FAILURE);
    }
    buffer_index++;

    intSet result = makeEmptyIntSet();
    while (buffer[buffer_index] != ']' && buffer_index < buffer_size) {
        // Next is a char
        if (buffer[buffer_index] == '\'' && (buffer_index+2 < buffer_size)) {
            char lookahead = buffer_index+3 <= buffer_size ? buffer[buffer_index+3] : 0;
            if (lookahead == '-') {
                // Found a multiple char statement
                if (buffer_index+6 < buffer_size) {
                    char first_char = buffer[buffer_index+1];
                    char last_char = buffer[buffer_index+5];
                    if (isValidChar(first_char)==FALSE || isValidChar(last_char)==FALSE) {
                        fprintf(stderr, "Error in multiple char statement in definitions section.\n");
                        exit (EXIT_FAILURE);
                    }
                    if (first_char > last_char) {
                        fprintf(stderr, "Error: multiple char statement with last char smaller than first char.\n");
                        exit(EXIT_FAILURE);
                    }
                    char current_char = first_char;
                    while (current_char <= last_char) {
                        if (isValidChar(current_char)) {
                            insertIntSet((unsigned int)current_char, &result);
                            current_char++;
                        }
                        else {
                            fprintf(stderr, "Error: invalid character in multiple char definition.\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    buffer_index += 6;
                    if (buffer[buffer_index] != '\'') {
                        fprintf(stderr, "Error: expected closing quote after char \"%c\".\n", buffer[buffer_index-1]);
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    fprintf(stderr, "Error: unfinished multiple char statement in definitions, usage: \'a\'-\'z\'.\n");
                    exit(EXIT_FAILURE);
                }
            }
            else {
                // Insert a single char
                buffer_index++;
                if (isValidChar(buffer[buffer_index])) {
                    insertIntSet((unsigned int)buffer[buffer_index], &result);
                }
                else {
                    fprintf(stderr, "Error: found invalid character in definition section.\n");
                    exit(EXIT_FAILURE);
                }
                buffer_index++;
                if (buffer[buffer_index] != '\'') {
                    fprintf(stderr, "Error: expected closing quote after char \"%c\".\n", buffer[buffer_index-1]);
                    exit(EXIT_FAILURE);
                }
            }
            buffer_index++;
        }
        else if (buffer[buffer_index] == ',') {
            buffer_index++;
        }
    }

    return result;
}

void printScannerDefinitions() {
    int i=0;
    for (i=0; i<definitions_count; i++) {
        printf("%s: ", definition_names[i]);
        printIntSet(definition_equivalences[i]);
        printf(";\n");
    }
    printf("\n");
}
