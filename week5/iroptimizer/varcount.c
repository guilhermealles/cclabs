#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "varcount.h"

static VarCountTable *var_count_table;

void initializeVarCountTable() {
    var_count_table = NULL;
}

void incrementUsesCount (char *var) {
    VarCountTable *table_ptr = var_count_table;
    
    while (table_ptr != NULL) {
        if (strcmp(table_ptr->var_name, var) == 0) {
            table_ptr->uses_count++;
            return;
        }
        table_ptr = table_ptr->next;
    }
    
    // TABLE IS EMPTY
    if (var_count_table == NULL) {
        var_count_table = malloc(sizeof(VarCountTable));
        var_count_table->var_name = stringDuplicate(var);
        var_count_table->uses_count = 1;
        var_count_table->next = NULL;
    }
    else {
        table_ptr = var_count_table;
        while (table_ptr->next != NULL) {
            table_ptr = table_ptr->next;
        }
        
        table_ptr->next = malloc(sizeof(VarCountTable));
        table_ptr = table_ptr->next;
        
        table_ptr->var_name = stringDuplicate(var);
        table_ptr->uses_count = 1;
        table_ptr->next = NULL;
    }
}

void resetUsesCount(char *var, int value) {
    VarCountTable *table_ptr = var_count_table;
    
    while (table_ptr != NULL) {
        if (strcmp(table_ptr->var_name, var) == 0) {
            table_ptr->uses_count = value;
            return;
        }
        table_ptr = table_ptr->next;
    }
    
    // TABLE IS EMPTY
    if (var_count_table == NULL) {
        var_count_table = malloc(sizeof(VarCountTable));
        var_count_table->var_name = stringDuplicate(var);
        var_count_table->uses_count = value;
        var_count_table->next = NULL;
    }
    else {
        table_ptr = var_count_table;
        while (table_ptr->next != NULL) {
            table_ptr = table_ptr->next;
        }
        
        table_ptr->next = malloc(sizeof(VarCountTable));
        table_ptr = table_ptr->next;
        table_ptr->var_name = stringDuplicate(var);
        table_ptr->uses_count = value;
        table_ptr->next = NULL;
    }
}

int getUsesCount(char *var) {
    VarCountTable *table_ptr = var_count_table;
    
    while (table_ptr != NULL) {
        if (strcmp(table_ptr->var_name, var) == 0) {
            return table_ptr->uses_count;
        }
        table_ptr = table_ptr->next;
    }
    
    return 0;
}

int existsInVarCountTable(char *var) {
    VarCountTable *table_ptr = var_count_table;
    
    while (table_ptr != NULL) {
        if (strcmp(table_ptr->var_name, var)== 0) {
            return 1;
        }
        table_ptr = table_ptr->next;
    }
    
    return 0;
}

void destroyVarCountTable() {
    while (var_count_table != NULL) {
        VarCountTable *next = var_count_table->next;
        free(var_count_table->var_name);
        free(var_count_table);
        var_count_table = next;
    }
}