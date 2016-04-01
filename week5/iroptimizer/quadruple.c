#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quadruple.h"
#include "misc.h"

static quadrupleQueue *quad_queue = NULL;

quadruple makeQuadruple(char *lhs, operator op, char *op1, char *op2) {
    quadruple q;
    q.lhs = stringDuplicate(lhs);
    q.operation = op;
    q.operand1 = stringDuplicate(op1);
    q.operand2 = stringDuplicate(op2);
    return q;
}

quadruple* duplicateQuadruple(quadruple quad) {
    quadruple *q = malloc(sizeof(quadruple));
    q->lhs = stringDuplicate(quad.lhs);
    q->operation = quad.operation;
    q->operand1 = stringDuplicate(quad.operand1);
    q->operand2 = stringDuplicate(quad.operand2);
    
    return q;
}

void freeQuadruple(quadruple q) {
    free(q.lhs);
    free(q.operand1);
    free(q.operand2);
}

void fprintfQuadruple(FILE *f, quadruple q) {
    fprintf(f, "%s=", q.lhs);
    fprintf(f, "%s", q.operand1);
    switch (q.operation) {
        case ASSIGNMENT: break;
        case PLUSOP    : fprintf(f, "+"); break;
        case MINUSOP   : fprintf(f, "-"); break;
        case TIMESOP   : fprintf(f, "*"); break;
        case DIVOP     : fprintf(f, "/"); break;
    }
    if (q.operation != ASSIGNMENT) {
        fprintf(f, "%s", q.operand2);
    }
    fprintf(f, ";");
}

int isEqualQuadruple(quadruple quad1, quadruple quad2) {
    if (strcmp(quad1.lhs, quad2.lhs) != 0) {
        return 0;
    }
    if (quad1.operation != quad2.operation) {
        return 0;
    }
    if (strcmp(quad1.operand1, quad2.operand2) != 0) {
        return 0;
    }
    if (strcmp(quad1.operand2, quad2.operand2) != 0) {
        return 0;
    }
    
    return 1;
}

void initializeQuadrupleQueue() {
    destroyQuadrupleQueue();
    quad_queue = NULL;
}

quadrupleQueue* getQuadrupleQueuePointer() {
    return quad_queue;
}

void destroyQuadrupleQueue() {
    if (quad_queue != NULL) {
        quadrupleQueue *next_ptr = quad_queue->next;
        
        while (next_ptr != NULL) {
            freeQuadruple(*quad_queue->quad);
            free(quad_queue);
            quad_queue = next_ptr;
            next_ptr = quad_queue->next;
        }
        
        freeQuadruple(*quad_queue->quad);
        free(quad_queue);
    }
}

// Returns the index of the new insertion
int insertQuadrupleInQueue(quadruple quad) {
    quadrupleQueue *queue_ptr = quad_queue;
    int index = 0;
    
    if (queue_ptr == NULL) {
        queue_ptr = malloc(sizeof(quadrupleQueue));
        quad_queue = queue_ptr;
    }
    else {
        while (queue_ptr->next != NULL) {
            queue_ptr = queue_ptr->next;
            index++;
        }
        queue_ptr->next = malloc(sizeof(quadrupleQueue));
        queue_ptr = queue_ptr->next;
    }
    
    queue_ptr->quad = duplicateQuadruple(quad);
    queue_ptr->removed_quadruple = 0;
    queue_ptr->rhs_index_consolidation = -1;
    
    queue_ptr->next = NULL;
    
    return index;
}

/*
 void removeQuadrupleFromQueue(quadruple quad) {
 quadrupleQueue *queue_ptr = quad_queue;
 
 if (queue_ptr != NULL) {
 // First element of the queue
 if (isEqualQuadruple(quad, *queue_ptr->quad)) {
 quad_queue = queue_ptr->next;
 freeQuadruple(*queue_ptr->quad);
 free(queue_ptr);
 }
 else {
 while (queue_ptr->next != NULL) {
 if (isEqualQuadruple(quad, *queue_ptr->next->quad)) {
 quadrupleQueue *next_next_ptr = queue_ptr->next->next;
 freeQuadruple(*queue_ptr->next->quad);
 free(queue_ptr->next);
 queue_ptr->next = next_next_ptr;
 }
 queue_ptr = queue_ptr->next;
 }
 }
 }
 }
 */

void removeQuadrupleFromQueueWithIndex(int index) {
    int i = 0;
    quadrupleQueue *queue_ptr = quad_queue;
    
    while (i!=index && queue_ptr != NULL) {
        queue_ptr = queue_ptr->next;
        i++;
    }
    
    if (i == index) {
        // Mark quadruple as removed.
        queue_ptr->removed_quadruple = 1;
    }
    else {
        fprintf(stderr, "Error: Queue index \"%d\" out of bounds.\n", index);
        exit(EXIT_FAILURE);
    }
}

int getQuadrupleIndex(quadruple quad) {
    int i = 0;
    quadrupleQueue *queue_ptr = quad_queue;
    
    while (queue_ptr != NULL) {
        if (isEqualQuadruple(quad, *queue_ptr->quad)) {
            return i;
        }
        i++;
        queue_ptr = queue_ptr->next;
    }
    
    return -1;
}

quadrupleQueue* getQuadrupleQueuePtrFromIndex(int index) {
    int i=0;
    quadrupleQueue *queue_ptr = quad_queue;
    
    while (queue_ptr != NULL && i < index) {
        queue_ptr = queue_ptr->next;
        i++;
    }
    return queue_ptr;
}

// given the index of a quadruple, remove all the consolidations (it should actually be only 1) of other quadruples with the rhs of this quadruple
void removeConsolidationScheduleFromIndex(int index) {
    quadrupleQueue *queue_ptr = quad_queue;
    
    while (queue_ptr != NULL) {
        if (queue_ptr->rhs_index_consolidation == index) {
            queue_ptr->rhs_index_consolidation = -1;
        }
        
        queue_ptr = queue_ptr->next;
    }
}


void fprintfQuadrupleQueue(FILE *f) {
    quadrupleQueue *queue_ptr = quad_queue;
    
    while (queue_ptr != NULL) {
        if (!queue_ptr->removed_quadruple) {
            fprintfQuadruple(f, *queue_ptr->quad);
            fprintf(f, "\n");
        }
        queue_ptr = queue_ptr->next;
    }
}
