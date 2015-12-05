#include <stdio.h>
#include "lib/intset.h"
#include "scanner_settings.h"
<<<<<<< HEAD
#include "scanner_regexps.h"
=======
#include "scanner_definitions.h"
>>>>>>> origin/master

int main (int argc, char **argv) {
    char *filename = "example2.lex";

    FILE *file = fopen(filename, "r");
    if (file) {
        char buf[255];
<<<<<<< HEAD
        fscanf(file, "%s", buf);
        fscanf(file, "%s", buf);
        parseScannerRegularExpressions(file);
        printRegularExpressionsData();
=======
        fscanf(file, "%s", buf); //section
        fscanf(file, "%s", buf); //options
        parseScannerOptions(file);
        printScannerOptions();
        printf("\n\n");
        fscanf(file, "%s", buf); //section
        fscanf(file, "%s", buf); //defines
        parseScannerDefinitions(file);
        printScannerDefinitions();

>>>>>>> origin/master
    }
    else {
        puts("puts");
    }

    return 0;
}
