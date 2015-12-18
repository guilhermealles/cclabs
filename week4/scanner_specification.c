#include "scanner_specification.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static ScannerOptions options_section;

static ScannerDefinition *definitions_section;

static unsigned int regex_trees_count = 0;
static RegexTree *regex_trees;

void initializeScannerOptions(){
    options_section.lexer_routine = "yylex";
    options_section.lexeme_name = "yytext";
    options_section.positioning_option = FALSE;
    options_section.positioning_line_name = NULL;
    options_section.positioning_column_name = NULL;
    options_section.default_action_routine = "defaultAction";
}

void setLexerRoutine(char *routine_name){
    free(options_section.lexer_routine);
    options_section.lexer_routine = malloc((strlen(routine_name) + 1) * sizeof(char));
    strcpy(options_section.lexer_routine, routine_name);
}

void setLexemeName(char *lexeme_name){
    free(options_section.lexeme_name);
    options_section.lexeme_name = malloc((strlen(lexeme_name) + 1) * sizeof(char));
    strcpy(options_section.lexeme_name, lexeme_name);
}

void setPositioningOption(int option){
    options_section.positioning_option = option;
}

void setPositioningLineName (char *name){
    free(options_section.positioning_line_name);
    options_section.positioning_line_name = malloc((strlen(name) + 1) * sizeof(char));
    strcpy(options_section.positioning_line_name, name);
}

void setPositioningColumneName (char *name){
    free(options_section.positioning_column_name);
    options_section.positioning_column_name = malloc((strlen(name) + 1) * sizeof(char));
    strcpy(options_section.positioning_column_name, name);
}

void setDefaultActionRoutineName(char *routine_name){
    free(options_section.default_action_routine);
    options_section.default_action_routine = malloc((strlen(routine_name) + 1) * sizeof(char));
    strcpy(options_section.default_action_routine, routine_name);
}

void printOptions(){
    printf("Lexer routine: %s\n", options_section.lexer_routine);
    printf("Lexeme name: %s\n", options_section.lexeme_name);
    printf("Positioning option: %d\n", options_section.positioning_option);

    if (options_section.positioning_option == TRUE){
        printf("Positioning line name: %s\n", options_section.positioning_line_name);
        printf("Positioning column name: %s\n", options_section.positioning_column_name);
    }

    printf("Default action routine: %s\n", options_section.default_action_routine);
}

void initializeDefinitionsSection() {
    definitions_section = NULL;
}

void mallocStrCpy(char *dest, char *src) {
    printf("Debug> Will copy %s (size %lu) to the definitions name.\n", src, (strlen(src)+1));
    dest = malloc(sizeof(char) * (strlen(src)+1));
    strcpy(dest, src);
    printf("Debug> Dest now contains %s.\n", dest);
}

ScannerDefinition* searchDefinition(char *name) {
    ScannerDefinition *definition_ptr = definitions_section;

    while (definition_ptr != NULL) {
        if (strcmp(definition_ptr->definition_name, name) == 0) {
            return definition_ptr;
        }
        definition_ptr = definition_ptr->next;
    }

    return NULL;
}

void addLiteralToDefinition (char *name, char *literal) {
    char literal_to_add = literal[1];

    ScannerDefinition *definition_ptr = searchDefinition(name);
    if (definition_ptr == NULL) {
        if (definitions_section == NULL) {
            definitions_section = malloc(sizeof(ScannerDefinition));
            definition_ptr = definitions_section;
        }
        else {
            definition_ptr = definitions_section;
            while (definition_ptr->next != NULL) {
                definition_ptr = definition_ptr->next;
            }
            definition_ptr->next = malloc(sizeof(ScannerDefinition));
            definition_ptr = definition_ptr->next;
        }
        definition_ptr->definition_name = malloc(sizeof(char) * (strlen(name)+1));
        strcpy(definition_ptr->definition_name, name);
        definition_ptr->definition_expansion = makeEmptyIntSet();
        insertIntSet((unsigned int)literal_to_add, &definition_ptr->definition_expansion);
        definition_ptr->next = NULL;
    }
    else {
        insertIntSet((unsigned int)literal_to_add, &definition_ptr->definition_expansion);
    }
}

void addRangeToDefinition (char *name, char *range) {
    char range_low = range[1];
    char range_high = range[5];
    if (range_low > range_high) {
        fprintf(stderr, "Error: definition of a range with lower bound greater than higher bound.\n");
        exit(EXIT_FAILURE);
    }

    ScannerDefinition *definition_ptr = searchDefinition(name);
    if (definition_ptr == NULL) {
        if (definitions_section == NULL) {
            definitions_section = malloc(sizeof(ScannerDefinition));
            definition_ptr = definitions_section;
        }
        else {
            definition_ptr = definitions_section;
            while (definition_ptr->next != NULL) {
                definition_ptr = definition_ptr->next;
            }
            definition_ptr->next = malloc(sizeof(ScannerDefinition));
            definition_ptr = definition_ptr->next;
        }
        definition_ptr->definition_name = malloc(sizeof(char) * (strlen(name)+1));
        strcpy(definition_ptr->definition_name, name);
        definition_ptr->definition_expansion = makeEmptyIntSet();
        int i;
        for (i=(int)range_low; i <= range_high; i++) {
            insertIntSet((unsigned int)i, &definition_ptr->definition_expansion);
        }
        definition_ptr->next = NULL;
    }
    else {
        int i;
        for (i=(int)range_low; i <= range_high; i++) {
            insertIntSet((unsigned int)i, &definition_ptr->definition_expansion);
        }
    }
}

void printDefinitions() {
    if (definitions_section == NULL) {
        printf("There are no definitions!\n");
    }
    else {
        ScannerDefinition *definition_ptr = definitions_section;
        while (definition_ptr != NULL) {
            printf("Definition name: %s.\n", definition_ptr->definition_name);
            printf("Definition expansion: "); printlnIntSet(definition_ptr->definition_expansion);
            definition_ptr = definition_ptr->next;
        }

        printf("\n");
    }
}

void initializeRegexTrees() {
    regex_trees_count = 0;
    regex_trees = malloc(sizeof(RegexTree) * regex_trees_count);
}

RegexTree* makeNewRegexTree() {
    RegexTree *tree = malloc(sizeof(RegexTree));
    tree->parent = NULL;
    tree->node_type = TYPE_REGEX;
    tree->regex_nfa = NULL;
    tree->children_count = 0;
    tree->children = NULL;

    return tree;
}

void makeRegexTreeNode(RegexTree *dest, int node_type, nfa *regex_nfa) {
    dest->parent = NULL;
    dest->node_type = node_type;
    if (dest->node_type == TYPE_VALUE) {
        // Add regex nfa
    }
    else {
        dest->regex_nfa = NULL;
    }
    dest->children_count = 0;
    dest->children = NULL;
}

// Return a pointer to the new child node
RegexTree *regexTreeAddTerm (RegexTree *node_to_add) {
    if (node_to_add->node_type != TYPE_REGEX) {
        fprintf(stderr, "Error: trying do add a term to a non-regex node. Node type: %d\n", node_to_add->node_type);
        exit(EXIT_FAILURE);
    }

    unsigned int new_children_index = node_to_add->children_count;
    node_to_add->children = realloc(node_to_add->children, sizeof(RegexTree) * (node_to_add->children_count+1));
    makeRegexTreeNode(&node_to_add->children[new_children_index], TYPE_TERM, NULL);
    node_to_add->children[new_children_index].parent = node_to_add;
    node_to_add->children_count++;

    return &node_to_add->children[new_children_index];
}

// Return a pointer to the new child node
RegexTree *regexTreeAddFactor (RegexTree *node_to_add) {
    if (node_to_add->node_type != TYPE_TERM) {
        fprintf(stderr, "Error: trying do add a factor to a non-term node. Node type: %d\n", node_to_add->node_type);
        exit(EXIT_FAILURE);
    }

    unsigned int new_children_index = node_to_add->children_count;
    node_to_add->children = realloc(node_to_add->children, sizeof(RegexTree) * (node_to_add->children_count+1));
    makeRegexTreeNode(&node_to_add->children[new_children_index], TYPE_FACTOR, NULL);
    node_to_add->children[new_children_index].parent = node_to_add;
    node_to_add->children_count++;

    return &node_to_add->children[new_children_index];
}

// Return a pointer to the new child node
RegexTree *regexTreeAddValue (RegexTree *node_to_add) {
    if (node_to_add->node_type != TYPE_FACTOR) {
        fprintf(stderr, "Error: trying do add a value to a non-factor node. Node type: %d\n", node_to_add->node_type);
        exit(EXIT_FAILURE);
    }

    unsigned int new_children_index = node_to_add->children_count;
    node_to_add->children = realloc(node_to_add->children, sizeof(RegexTree) * (node_to_add->children_count+1));
    makeRegexTreeNode(&node_to_add->children[new_children_index], TYPE_VALUE, NULL/* to-do */);
    node_to_add->children[new_children_index].parent = node_to_add;
    node_to_add->children_count++;

    return &node_to_add->children[new_children_index];
}

// Return a pointer to the new child node
RegexTree *regexTreeAddRegex (RegexTree *node_to_add) {
    if (node_to_add->node_type != TYPE_FACTOR) {
        fprintf(stderr, "Error: trying do add a regex to a non-factor node. Node type: %d\n", node_to_add->node_type);
        exit(EXIT_FAILURE);
    }

    unsigned int new_children_index = node_to_add->children_count;
    node_to_add->children = realloc(node_to_add->children, sizeof(RegexTree) * (node_to_add->children_count+1));
    makeRegexTreeNode(&node_to_add->children[new_children_index], TYPE_REGEX, NULL);
    node_to_add->children[new_children_index].parent = node_to_add;
    node_to_add->children_count++;

    return &node_to_add->children[new_children_index];
}

// Add a tree to the array, returns the index of the new tree.
unsigned int addTreeToArray (RegexTree *tree_to_add) {
    unsigned int new_tree_index = regex_trees_count;
    regex_trees = realloc(regex_trees, sizeof(RegexTree) * (regex_trees_count+1));
    regex_trees[new_tree_index] = *tree_to_add;
    regex_trees_count++;
    return new_tree_index;
}
