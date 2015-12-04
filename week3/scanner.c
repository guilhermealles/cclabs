#include <stdio.h>
#include "scanner_settings.h"

int main (int argc, char **argv) {
    char *filename = "example.lex";

    FILE *file = fopen(filename, "r");
    if (file) {
        char buf[255];
        fscanf(file, "%s", buf);
        fscanf(file, "%s", buf);
        parseScannerOptions(file);
        printScannerSettings();
    }
    else {
        puts("puts");
    }

    return 0;
}
