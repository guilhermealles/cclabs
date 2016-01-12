#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quadruple.h"
#include "misc.h"

extern int yyparse();
extern void initLexer(char *fnm);
extern void finalizeLexer();

typedef struct stringPair {
    char *key, *str;
} stringPair;

static stringPair *table = NULL;
static int tableSize=0;     /* number of pairs in the pair table */
static int allocatedSize=0; /* allocated number of table entries */

static void resizeStringTable() {
    allocatedSize = (allocatedSize == 0 ? 1 : 2*allocatedSize);
    table = safeRealloc(table, allocatedSize*sizeof(stringPair));
}

static int lookupInStringTable(char *key) {
    int i;
    for (i=0; i < tableSize; i++) {
        if (areEqualStrings(table[i].key, key)) {
            return i;
        }
    }
    return -1;  /* not found */
}

static void insertStringPair(char *key, char *str) {
    int idx = lookupInStringTable(key);
    if (idx != -1) {
        free(table[idx].str);
        table[idx].str = stringDuplicate(str);
        return;
    }
    if (tableSize == allocatedSize) {
        resizeStringTable();
    }
    table[tableSize].key = stringDuplicate(key);
    table[tableSize].str = stringDuplicate(str);
    tableSize++;
}

static void removeStringPairs(char *key) {
    /* remove any pair (x,y) where x==key or y==key from table */
    int i, idx;
    for (i=idx=0; i < tableSize; i++) {
        if (areEqualStrings(table[i].key, key) ||
            areEqualStrings(table[i].str, key)) {
            free(table[i].key);
            free(table[i].str);
        } else {
            table[idx++] = table[i];
        }
    }
    tableSize = idx;
}

static void deallocateTable() {
    int i;
    for (i=0; i < tableSize; i++) {
        free(table[i].key);
        free(table[i].str);
    }
    free(table);
    table = NULL;
    tableSize = 0;
    allocatedSize = 0;
}


/********************************************************************/
char* replace(char *operand) {
    int stringTableIndex = lookupInStringTable(operand);
    if (stringTableIndex == -1) {
        return operand;
    }
    else {
        free(operand);
        char *returnstr = malloc(sizeof(char) * (strlen(table[stringTableIndex].str) + 1));
        strcpy(returnstr, table[stringTableIndex].str);
        return returnstr;
    }
}

int isConstant(char *operand) {
    if (operand[0] > 47 && operand[0] < 58) {
        return 1;
    }
    else {
        return 0;
    }
}

int calculateQuadruple(quadruple quad) {
    int operand1 = atoi(quad.operand1);
    int operand2 = atoi(quad.operand2);

    int result;
    switch(quad.operation) {
        case PLUSOP:
            result = operand1 + operand2;
            break;
        case MINUSOP:
            result = operand1 - operand2;
            break;
        case TIMESOP:
            result = operand1 * operand2;
            break;
        case DIVOP:
            if (operand2 == 0) {
                fprintf(stderr, "Error: division by zero!\n");
                exit(EXIT_FAILURE);
            }
            result = operand1 / operand2;
            break;
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
            break;
    }
    return result;
}

void processQuadruple(quadruple quad) {

    if (quad.operation == ASSIGNMENT) {
        quad.operand1 = replace(quad.operand1);
        removeStringPairs(quad.lhs);
        insertStringPair(quad.lhs, quad.operand1);
    }
    else {
        quad.operand1 = replace(quad.operand1);
        quad.operand2 = replace(quad.operand2);
        removeStringPairs(quad.lhs);

        if (isConstant(quad.operand1) && isConstant(quad.operand2)) {
            int result = calculateQuadruple(quad);
            char *resultstr = malloc(sizeof(char) * 11);
            sprintf(resultstr, "%d", result);
            quad.operation = ASSIGNMENT;
            strcpy(quad.operand1, resultstr);
            free(resultstr);

            insertStringPair(quad.lhs, quad.operand1);
        }
    }

    fprintfQuadruple(stdout, quad);
    fprintf(stdout, "\n");
    freeQuadruple(quad);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        abortMessage("Usage: %s <program.ir>", argv[0]);
    }

    initLexer(argv[1]);
    yyparse();
    finalizeLexer();
    deallocateTable();

    return EXIT_SUCCESS;
}
