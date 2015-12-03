#include <stdio.h>
#include "scanner_settings.h"

int main (int argc, char **argv) {
    char *filename = "example.lex";

    FILE *file = fopen(filename, "r");
    if (file) {
        parseScannerOptions(file);
        printScannerSettings();
    }
    else {
        puts("puts");
    }

    return EXIT_SUCCESS;
}
