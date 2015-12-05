#include <stdio.h>
#include "lib/intset.h"
#include "scanner_settings.h"
#include "scanner_definitions.h"

int main (int argc, char **argv) {
    char *filename = "example.lex";

    FILE *file = fopen(filename, "r");
    if (file) {
        char buf[255];
        fscanf(file, "%s", buf); //section
        fscanf(file, "%s", buf); //options
        parseScannerOptions(file);
        printScannerOptions();
        printf("\n\n");
        fscanf(file, "%s", buf); //section
        fscanf(file, "%s", buf); //defines
        parseScannerDefinitions(file);
        printScannerDefinitions();

    }
    else {
        puts("puts");
    }

    return 0;
}
