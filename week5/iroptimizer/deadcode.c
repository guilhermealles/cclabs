#include "deadcode.h"
#include "quadruple.h"
#include "misc.h"
#include "varcount.h"
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
// This function returns the definition index of the variable if resurrected, and -1 if the variable was not found.
int resurrectVariable(char *var_name) {
    DeadVarsList *list_ptr = dead_variables;
    int index = 0;
    if (list_ptr != NULL) {
        // First element of the list
        if (strcmp(var_name, list_ptr->var_name) == 0) {
            int def_index = list_ptr->definition_index;
            dead_variables = dead_variables->next;
            free(list_ptr->var_name);
            free(list_ptr);
            return def_index;
        }
        else {
            while (list_ptr->next != NULL) {
                if (strcmp(var_name, list_ptr->next->var_name) == 0) {
                    int def_index = list_ptr->next->definition_index;
                    DeadVarsList *next_next_ptr = list_ptr->next->next;
                    free(list_ptr->next->var_name);
                    free(list_ptr->next);
                    list_ptr->next = next_next_ptr;
                    return def_index;
                }
                else {
                    list_ptr = list_ptr->next;
                    index++;
                }
            }
        }
    }
    return -1;
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

// This function gets the two quadruples and substitute them for one of the type lhs = rhs.
// The quadruple corresponding to the rhs is removed from the quadrupleQueue.
void consolidateQuadruplesInQuadQueue(int lhs_index, int rhs_index) {
    quadrupleQueue *lhs_quad_queue = getQuadrupleQueuePtrFromIndex(lhs_index);
    quadrupleQueue *rhs_quad_queue = getQuadrupleQueuePtrFromIndex(rhs_index);

    if (lhs_quad_queue != NULL && rhs_quad_queue != NULL) {
        if (getUsesCount(rhs_quad_queue->quad->lhs) == 1) {
            lhs_quad_queue->quad->operation = rhs_quad_queue->quad->operation;
            lhs_quad_queue->quad->operand1 = stringDuplicate(rhs_quad_queue->quad->operand1);
            if (rhs_quad_queue->quad->operation != ASSIGNMENT) {
                lhs_quad_queue->quad->operand2 = stringDuplicate(rhs_quad_queue->quad->operand2);
            }

            removeQuadrupleFromQueueWithIndex(rhs_index);
        }
    }
    else {
        fprintf(stderr, "Error: invalid indexes on consolidateQuadruplesInQuadQueue()!\n");
        exit(EXIT_FAILURE);
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

void runRedundancyConsolidation() {
    // Redundancy consolidation uses the dead variables list without actually deleting any code.
    initializeVarCountTable();
    initializeDeadVarsList();
    quadrupleQueue *quad_queue = getQuadrupleQueuePointer();
    int index = 0;

    while (quad_queue != NULL) {
        if (quad_queue->removed_quadruple == 0) {
            resetUsesCount(quad_queue->quad->lhs, 0);
            insertDeadVariable(quad_queue->quad->lhs, index);
            if (quad_queue->quad->operation == ASSIGNMENT) {
                int operand_index = resurrectVariable(quad_queue->quad->operand1);
                quad_queue->rhs_index_consolidation = operand_index;
                incrementUsesCount(quad_queue->quad->operand1);
            }
            else {
                resurrectVariable(quad_queue->quad->operand1);
                resurrectVariable(quad_queue->quad->operand2);
                incrementUsesCount(quad_queue->quad->operand1);
                incrementUsesCount(quad_queue->quad->operand2);
            }
        }

        index++;
        quad_queue = quad_queue->next;
    }

    // Run the consolidation to remove redundant assignments
    quad_queue = getQuadrupleQueuePointer();
    index = 0;
    while (quad_queue != NULL) {
        if (quad_queue->removed_quadruple == 0) {
            int consolidation_index = quad_queue->rhs_index_consolidation;
            if (consolidation_index != -1) {
                consolidateQuadruplesInQuadQueue(index, consolidation_index);
            }
        }
        quad_queue = quad_queue->next;
        index++;
    }

    destroyDeadVarsList();
    destroyVarCountTable();
}

void runDeadCodeElimination() {
    runRedundancyConsolidation();

    int removal_count = -1;
    while (removal_count != 0) {
        removal_count = 0;
        // Run the dead code elimination until the code stabilizes

        initializeDeadVarsList();
        quadrupleQueue *quad_queue = getQuadrupleQueuePointer();
        int index = 0;

        while (quad_queue != NULL) {
            if (quad_queue->removed_quadruple == 0) {
                if (lookupDeadVariable(quad_queue->quad->lhs) != -1) {
                    // If the left hand side is marked as dead, remove it from the original code
                    removeDeadVariable(quad_queue->quad->lhs);
                    removal_count++;
                }
                insertDeadVariable(quad_queue->quad->lhs, index);
                if (quad_queue->quad->operation == ASSIGNMENT) {
                    resurrectVariable(quad_queue->quad->operand1);
                }
                else {
                    resurrectVariable(quad_queue->quad->operand1);
                    resurrectVariable(quad_queue->quad->operand2);
                }
            }

            quad_queue = quad_queue->next;
            index++;
        }
        destroyDeadVarsList();
    }
}
