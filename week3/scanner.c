#include <stdio.h>
#include "scanner_settings.h"
#include "scanner_regexps.h"

int main (int argc, char **argv) {
    char *filename = "example2.lex";

    FILE *file = fopen(filename, "r");
    if (file) {
        char buf[255];
        fscanf(file, "%s", buf);
        fscanf(file, "%s", buf);
        parseScannerRegularExpressions(file);
        printRegularExpressionsData();
    }
    else {
        puts("puts");
    }

    return 0;
}
