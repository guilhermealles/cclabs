#ifndef varcount_h
#define varcount_h

typedef struct VarCountTable {
    char *var_name;
    int uses_count;
    
    struct VarCountTable *next;
} VarCountTable;

void initializeVarCountTable();
void incrementUsesCount (char *var);
void resetUsesCount(char *var, int value);
int getUsesCount(char *var);
int existsInVarCountTable(char *var);
void destroyVarCountTable();


#endif /* varcount_h */
