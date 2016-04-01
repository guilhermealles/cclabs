#ifndef DEADCODE_H
#define DEADCODE_H

typedef struct DeadVarsList {
    char *var_name;
    int definition_index;
    
    struct DeadVarsList *next;
} DeadVarsList;

void initializeDeadVarsList();
void insertDeadVariable(char *var_name, int definition_index);
int lookupDeadVariable(char *var_name);
int resurrectVariable(char *var_name);
void removeDeadVariable(char *var_name);
void consolidateQuadruplesInQuadQueue(int lhs_index, int rhs_index);
void destroyDeadVarsList();

void runRedundancyConsolidation();
void runDeadCodeElimination();

#endif
