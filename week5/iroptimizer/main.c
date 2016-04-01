#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quadruple.h"
#include "deadcode.h"
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

static quadruple *subexp_table = NULL;
static int subexp_table_size=0;
static int subexp_table_alocated_size = 0;
static int temp_variable_count = 1;

static void resizeStringTable() {
    allocatedSize = (allocatedSize == 0 ? 1 : 2*allocatedSize);
    table = safeRealloc(table, allocatedSize*sizeof(stringPair));
}

static void resizeSubexpressionTable(){
    subexp_table_alocated_size = (subexp_table_alocated_size == 0 ? 1 : 2 * subexp_table_alocated_size);
    subexp_table = safeRealloc(subexp_table, subexp_table_alocated_size * sizeof(quadruple));
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

static int areEqualSubexpressions(quadruple quad1, quadruple quad2){
    if (quad1.operation == quad2.operation){
        if (areEqualStrings(quad1.operand1, quad2.operand1)){
            if (areEqualStrings(quad1.operand2, quad2.operand2)){
                return 1;
            }
        }
        else if(quad1.operation == PLUSOP || quad1.operation == TIMESOP){
            // Considers commutativity
            if (areEqualStrings(quad1.operand1, quad2.operand2)){
                if (areEqualStrings(quad1.operand2, quad2.operand1)){
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int lookupInSubexpressionTable(quadruple q){
    int i;
    for (i = 0; i < subexp_table_size; i++){
        if (areEqualSubexpressions(subexp_table[i], q)){
            return i;
        }
    }
    return -1; // not found
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

static void insertSubexpression(quadruple quad){
    int index = lookupInSubexpressionTable(quad);

    if (index == -1){
        if (subexp_table_size == subexp_table_alocated_size){
            resizeSubexpressionTable();
        }
        char *temp_name = malloc(sizeof(char)*12);
        sprintf(temp_name, "_%d", temp_variable_count);
        quadruple q;
        q = makeQuadruple(temp_name, quad.operation, quad.operand1, quad.operand2);

        subexp_table[subexp_table_size] = q;
        subexp_table_size++;
        free(temp_name);
        temp_variable_count++;
    }
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

/* Remove an expression from the table if any operand is changed. */
static void removeSubexpressionPairs(char *operand){
    int i, index;

    for(i = index = 0; i < subexp_table_size; i++){
        if (areEqualStrings(subexp_table[i].operand1, operand) ||
            areEqualStrings(subexp_table[i].operand2, operand)){
            freeQuadruple(subexp_table[i]);
        }
        else{
            subexp_table[index++] = subexp_table[i];
        }
    }
    subexp_table_size = index;
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

static void deallocateSubexpressionTable(){
    int i;
    for (i = 0; i < subexp_table_size; i++){
        freeQuadruple(subexp_table[i]);
    }
    free(subexp_table);
    subexp_table = NULL;
    subexp_table_size = 0;
    subexp_table_alocated_size = 0;
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
    /* Copy propagation. */
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

    /* Common subexpression elimination. */

    /* If one of the operands is equal to the LHS, there's no need to look for
     it on the table. Also, if the expression is a unary operation (i.e. a = b),
     just copy it.*/
    if((quad.operand2 != NULL)
       && !(areEqualStrings(quad.lhs, quad.operand1) ||
            areEqualStrings(quad.lhs, quad.operand2))){

           int index;
           index = lookupInSubexpressionTable(quad);

           if (index == -1){
               insertSubexpression(quad);
               insertQuadrupleInQueue(subexp_table[subexp_table_size-1]);
               index = subexp_table_size-1;
           }
           quad.operation = ASSIGNMENT;
           free(quad.operand1);
           quad.operand1 = malloc(sizeof(char) * strlen(subexp_table[index].lhs)+1);
           strcpy(quad.operand1, subexp_table[index].lhs);
       }
    removeSubexpressionPairs(quad.lhs);

    insertQuadrupleInQueue(quad);
    freeQuadruple(quad);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        abortMessage("Usage: %s <program.ir>", argv[0]);
    }

    initializeQuadrupleQueue();
    initLexer(argv[1]);

    yyparse();
    runDeadCodeElimination();

    fprintfQuadrupleQueue(stdout);
    destroyQuadrupleQueue();

    finalizeLexer();
    deallocateTable();
    deallocateSubexpressionTable();

    return EXIT_SUCCESS;
}
