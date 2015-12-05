#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lib/intset.h"
#include "scanner_settings.h"
#include "scanner_regexps.h"
#include "scanner_definitions.h"

void parseSpecificationFile(FILE *file);

int main (int argc, char **argv) {
    char *filename = "example.lex";

    FILE *file = fopen(filename, "r");
    if (file) {
        parseSpecificationFile(file);
    }
    else {
        fprintf(stderr, "Error: unable to open specification file \"%s\".\n", filename);
        exit(EXIT_FAILURE);
    }
    printScannerOptions();
    printf("\n\n");
    printScannerDefinitions();
    printf("\n\n");
    printRegularExpressionsData();

    return 0;
}

void parseSpecificationFile(FILE *file) {
    char *buffer = malloc(sizeof(char) * 255);

    while (fscanf(file, "%s", buffer) != EOF) {
        if (strcmp(buffer, "section") == 0) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "options") == 0) {
                parseScannerOptions(file);
            }
            else if (strcmp(buffer, "defines") == 0) {
                parseScannerDefinitions(file);
            }
            else if (strcmp(buffer, "regexps") == 0) {
                parseScannerRegularExpressions(file);
            }
            else {
                fprintf(stderr, "Error: expected section \"options\", \"defines\" or \"regexps\". Found \"%s\".\n", buffer);
                exit(EXIT_FAILURE);
            }
        }
        else {
            fprintf(stderr, "Error: expected keyword \"section\", found \"%s\".\n", buffer);
            exit(EXIT_FAILURE);
        }
    }
}
