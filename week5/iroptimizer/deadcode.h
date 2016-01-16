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
void resurrectVariable(char *var_name);
void removeDeadVariable(char *var_name);
void destroyDeadVarsList();

void runDeadCodeElimination();

#endif
