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

typedef struct quadrupleQueue {
    quadruple *quad;
    int removed_quadruple;
    struct quadrupleQueue *next;
} quadrupleQueue;

quadruple makeQuadruple(char *lhs, operator op, char *op1, char *op2);
quadruple* duplicateQuadruple(quadruple quad);
void freeQuadruple(quadruple q);
void fprintfQuadruple(FILE *f, quadruple q);

int isEqualQuadruple(quadruple quad1, quadruple quad2);

// Queue operations for quadruples
void initializeQuadrupleQueue();
quadrupleQueue* getQuadrupleQueuePointer();
int insertQuadrupleInQueue(quadruple quad);
void removeQuadrupleFromQueue(quadruple quad);
void removeQuadrupleFromQueueWithIndex(int index);
void destroyQuadrupleQueue();
int getQuadrupleIndex(quadruple quad);
void fprintfQuadrupleQueue(FILE *f);

#endif
