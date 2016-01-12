#ifndef QUADRUPLE_H
#define QUADRUPLE_H

#include <stdio.h>

typedef enum operator {
    ASSIGNMENT, PLUSOP, MINUSOP, TIMESOP, DIVOP
} operator;

typedef struct quadruple {
    char *lhs;
    operator operation;
    char *operand1;
    char *operand2; /* not used if operation == ASSIGNMENT */
} quadruple;

quadruple makeQuadruple(char *lhs, operator op, char *op1, char *op2);
void freeQuadruple(quadruple q);
void fprintfQuadruple(FILE *f, quadruple q);

#endif
