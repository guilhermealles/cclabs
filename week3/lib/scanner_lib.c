#include <stdio.h>
#include <stdlib.h>
#include "scanner_lib.h"

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
int isValidChar(char c) {
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
