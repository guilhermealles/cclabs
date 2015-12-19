#ifndef SCANNER_SPECIFICATION
#define SCANNER_SPECIFICATION

#include <stdio.h>
#include "intset.h"
#include "nfa.h"

#define TRUE 1
#define FALSE 0

#define TYPE_REGEX 1
#define TYPE_TERM 2
#define TYPE_FACTOR 3

#define BINARYOP_UNION 4
#define BINARYOP_CONCATENATION 5
#define UNARYOP_OPTIONAL 6
#define UNARYOP_KLEENECLOSURE 7
#define UNARYOP_POSITIVECLOSURE 8

#define TYPE_VALUE 9

typedef struct ScannerOptions{
    char *lexer_routine;
    char *lexeme_name;
    int positioning_option;
    char *positioning_line_name;
    char *positioning_column_name;
    char *default_action_routine;
}ScannerOptions;

typedef struct ScannerDefinition {
    char *definition_name;
    intSet definition_expansion;
    struct ScannerDefinition *next;
} ScannerDefinition;

typedef struct RegexTree {
    struct RegexTree *parent;
    unsigned int node_type;
    nfa regex_nfa;
    unsigned int children_count;
    struct RegexTree *children;
} RegexTree;

void initializeScannerOptions();
void setLexerRoutine(char *routine_name);
void setLexemeName(char *lexeme_name);
void setPositioningOption(int option);
void setPositioningLineName (char *name);
void setPositioningColumneName (char *name);
void setDefaultActionRoutineName(char *routine_name);
void printOptions();

void initializeDefinitionsSection();
void mallocStrCpy(char *dest, char *src);
ScannerDefinition* searchDefinition(char *name);
void addLiteralToDefinition(char *name, char *literal);
void addRangeToDefinition(char *name, char *range);
void printDefinitions();

void initializeRegexTrees();
RegexTree* makeNewRegexTree();
void makeRegexTreeNode(RegexTree *dest, int node_type);
void makeRegexTreeValueNode(RegexTree *dest, nfa regex_nfa);
RegexTree *regexTreeAddTerm (RegexTree *node_to_add);
RegexTree *regexTreeAddFactor (RegexTree *node_to_add);
RegexTree *regexTreeAddValue (RegexTree *node_to_add, char *regex_value);
RegexTree *regexTreeAddRegex (RegexTree *node_to_add);
RegexTree* regexTreeAddBinary (RegexTree *node_to_add, int binary_op);
RegexTree* regexTreeAddUnary (RegexTree *node_to_add, int unary_op);
unsigned int addTreeToArray (RegexTree *tree_to_add);



#endif
