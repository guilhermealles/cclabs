#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "misc.h"

void abortMessage(char *format, ...) {
    va_list argp;
    va_start(argp, format);
    vfprintf(stderr, format, argp);
    va_end(argp);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void *safeMalloc(int nbytes) {
    void *ptr = malloc(nbytes);
    if (ptr == NULL) {
        abortMessage("Error: memory allocation failed "
                     "for safeMalloc(%d).", nbytes);
    }
    return ptr;
}

void *safeRealloc(void *ptr, int nbytes) {
    ptr = realloc(ptr, nbytes);
    if (ptr == NULL) {
        abortMessage("Error: memory allocation failed "
                     "for safeRealloc(%d).", nbytes);
    }
    return ptr;
}

char *readFile(char *fnm) {
    FILE *f = fopen(fnm, "r");
    unsigned int size, idx = 0;
    char *text;
    if (f == NULL) {
        abortMessage("Error: Failed to open file [%s]", fnm);
    }
    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    rewind(f);
    text = safeMalloc(1 + size);
    while (idx < size) {
        idx += fread(&text[idx], 1, size-idx, f);
    }
    text[size] = '\0';
    fclose(f);
    return text;
}

char *stringDuplicate(char *str) {
    if (str == NULL) {
        return NULL;
    }
    return strcpy(safeMalloc(1+strlen(str)), str);
}

int areEqualStrings(char *s1, char *s2) {
    return (strcmp(s1, s2) == 0);
}
