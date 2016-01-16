#include "deadcode.h"
#include "quadruple.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>

static DeadVarsList *dead_variables;

void initializeDeadVarsList() {
    destroyDeadVarsList();
    dead_variables = NULL;
}

void insertDeadVariable(char *var_name, int definition_index){
    DeadVarsList *list_ptr = dead_variables;

    if (list_ptr == NULL) {
        list_ptr = malloc(sizeof(DeadVarsList));
        dead_variables = list_ptr;
    }
    else {
        while (list_ptr->next != NULL) {
            list_ptr = list_ptr->next;
        }
        list_ptr->next = malloc(sizeof(DeadVarsList));
        list_ptr = list_ptr->next;
    }

    list_ptr->var_name = stringDuplicate(var_name);
    list_ptr->definition_index = definition_index;
    list_ptr->next = NULL;
}

// returns the index of the variable if found, -1 otherwise.
int lookupDeadVariable(char *var_name) {
    DeadVarsList *list_ptr = dead_variables;
    int found = 0;
    int index = 0;

    while (list_ptr != NULL && found != 1) {
        if (strcmp(var_name, list_ptr->var_name) == 0) {
            found = 1;
        }
        else {
            index++;
            list_ptr = list_ptr->next;
        }
    }
    index = found ? index : -1;

    return index;
}

// Remove variable from Dead List, but does not remove it from the quadruples queue.
void resurrectVariable(char *var_name) {
    DeadVarsList *list_ptr = dead_variables;
    if (list_ptr != NULL) {
        // First element of the list
        if (strcmp(var_name, list_ptr->var_name) == 0) {
            dead_variables = dead_variables->next;
            free(list_ptr->var_name);
            free(list_ptr);
        }
        else {
            int found = 0;
            while (list_ptr->next != NULL && found != 1) {
                if (strcmp(var_name, list_ptr->next->var_name) == 0) {
                    found = 1;
                    DeadVarsList *next_next_ptr = list_ptr->next->next;
                    free(list_ptr->next->var_name);
                    free(list_ptr->next);
                    list_ptr->next = next_next_ptr;
                }
                else {
                    list_ptr = list_ptr->next;
                }
            }
        }
    }
}

// Mark the dead variable as 'REMOVED' in the quadruples queue and removes it from the dead variables list
void removeDeadVariable(char *var_name) {
    DeadVarsList *list_ptr = dead_variables;
    if (list_ptr != NULL) {
        // First element of the list
        if (strcmp(var_name, list_ptr->var_name) == 0) {
            removeQuadrupleFromQueueWithIndex(list_ptr->definition_index);
            dead_variables = dead_variables->next;
            free(list_ptr->var_name);
            free(list_ptr);
        }
        else {
            int found = 0;
            while (list_ptr->next != NULL && found != 1) {
                if (strcmp(var_name, list_ptr->next->var_name) == 0) {
                    found = 1;
                    int quadruple_queue_index = list_ptr->next->definition_index;
                    removeQuadrupleFromQueueWithIndex(quadruple_queue_index);
                    DeadVarsList *next_next_ptr = list_ptr->next->next;
                    free(list_ptr->next->var_name);
                    free(list_ptr->next);
                    list_ptr->next = next_next_ptr;
                }
                else {
                    list_ptr = list_ptr->next;
                }
            }
        }
    }
}

void destroyDeadVarsList() {
    DeadVarsList *list_ptr = dead_variables;

    while (list_ptr != NULL) {
        dead_variables = list_ptr->next;
        free(list_ptr->var_name);
        free(list_ptr);
        list_ptr = dead_variables;
    }
}

void runDeadCodeElimination() {
    initializeDeadVarsList();

    quadrupleQueue *quad_queue = getQuadrupleQueuePointer();
    int index = 0;

    while (quad_queue != NULL) {
        if (lookupDeadVariable(quad_queue->quad->lhs) != -1) {
            // If the left hand side is marked as dead, remove it from the original code
            removeDeadVariable(quad_queue->quad->lhs);
        }
        insertDeadVariable(quad_queue->quad->lhs, index);
        if (quad_queue->quad->operation == ASSIGNMENT) {
            resurrectVariable(quad_queue->quad->operand1);
        }
        else {
            resurrectVariable(quad_queue->quad->operand1);
            resurrectVariable(quad_queue->quad->operand2);
        }

        quad_queue = quad_queue->next;
        index++;
    }
}
