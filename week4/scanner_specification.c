#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "nfa.h"
#include "scanner_specification.h"

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

int parseOperationsToType (char *lexeme) {
    char op = lexeme[0];

    switch(op) {
        case '|':
            return BINARYOP_UNION;
        case '.':
            return BINARYOP_CONCATENATION;
        case '?':
            return UNARYOP_OPTIONAL;
        case '*':
            return UNARYOP_KLEENECLOSURE;
        case '+':
            return UNARYOP_POSITIVECLOSURE;
        default:
            fprintf(stderr, "Error: unrecognised operator \'%s\'.\n", lexeme);
            exit(EXIT_FAILURE);
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
    tree->children_count = 0;
    tree->children = NULL;

    return tree;
}

void makeRegexTreeNode(RegexTree *dest, int node_type) {
    if (node_type == TYPE_VALUE) {
        fprintf(stderr, "Error: trying to add a value node with makeRegexTreeNode().\n");
        exit(EXIT_FAILURE);
    }
    dest->parent = NULL;
    dest->node_type = node_type;
    dest->children_count = 0;
    dest->children = NULL;
}

void makeRegexTreeValueNode(RegexTree *dest, nfa regex_nfa) {
    dest->parent = NULL;
    dest->node_type = TYPE_VALUE;
    dest->regex_nfa = regex_nfa;
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
    makeRegexTreeNode(&node_to_add->children[new_children_index], TYPE_TERM);
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
    makeRegexTreeNode(&node_to_add->children[new_children_index], TYPE_FACTOR);
    node_to_add->children[new_children_index].parent = node_to_add;
    node_to_add->children_count++;

    return &node_to_add->children[new_children_index];
}

// Return a pointer to the new child node
RegexTree *regexTreeAddValue (RegexTree *node_to_add, char *regex_value) {
    if (node_to_add->node_type != TYPE_FACTOR) {
        fprintf(stderr, "Error: trying do add a value to a non-factor node. Node type: %d\n", node_to_add->node_type);
        exit(EXIT_FAILURE);
    }

    unsigned int new_children_index = node_to_add->children_count;
    node_to_add->children = realloc(node_to_add->children, sizeof(RegexTree) * (node_to_add->children_count+1));
    nfa regex_nfa = regexpToNFA(regex_value);
    makeRegexTreeValueNode(&node_to_add->children[new_children_index], regex_nfa);
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
    makeRegexTreeNode(&node_to_add->children[new_children_index], TYPE_REGEX);
    node_to_add->children[new_children_index].parent = node_to_add;
    node_to_add->children_count++;

    return &node_to_add->children[new_children_index];
}

RegexTree* regexTreeAddBinary (RegexTree *node_to_add, int binary_op) {
    if (node_to_add->node_type != TYPE_REGEX) {
        fprintf(stderr, "Error: trying to add a binary operation to a non-regex node.\n");
        exit(EXIT_FAILURE);
    }

    unsigned int new_children_index = node_to_add->children_count;
    node_to_add->children = realloc(node_to_add->children, sizeof(RegexTree) * (node_to_add->children_count+1));
    makeRegexTreeNode(&node_to_add->children[new_children_index], binary_op);
    node_to_add->children[new_children_index].parent = node_to_add;
    node_to_add->children_count++;

    return &node_to_add->children[new_children_index];
}

RegexTree* regexTreeAddUnary (RegexTree *node_to_add, int unary_op){
    if (node_to_add->node_type != TYPE_TERM) {
        fprintf(stderr, "Error: trying to add a binary operation to a non-term node.\n");
        exit(EXIT_FAILURE);
    }

    unsigned int new_children_index = node_to_add->children_count;
    node_to_add->children = realloc(node_to_add->children, sizeof(RegexTree) * (node_to_add->children_count+1));
    makeRegexTreeNode(&node_to_add->children[new_children_index], unary_op);
    node_to_add->children[new_children_index].parent = node_to_add;
    node_to_add->children_count++;

    return &node_to_add->children[new_children_index];
}

void evaluateRegexTree(RegexTree *root) {
    evaluateRegexTreeRec(root);
}

nfa evaluateRegexTreeRec(RegexTree *tree) {
    nfa final_nfa;
    switch(tree->node_type) {
        case TYPE_REGEX:
            final_nfa = evaluateRegexTreeRec(&tree->children[0]);
            int children_index = 0;
            while (children_index < (tree->children_count-1)) {
                // Switch the binary operation
                switch(tree->children[children_index+1].node_type) {
                    case BINARYOP_UNION:
                        final_nfa = uniteNFAs(final_nfa, evaluateRegexTreeRec(&tree->children[children_index+2]));
                        break;
                    case BINARYOP_CONCATENATION:
                        final_nfa = concatenateNFAs(final_nfa, evaluateRegexTreeRec(&tree->children[children_index+2]));
                        break;
                    default:
                        fprintf(stderr, "Error in evaluateRegexTreeRec.\n");
                        exit(EXIT_FAILURE);
                }
                children_index += 2;
            }

            tree->regex_nfa = final_nfa;
            break;
        case TYPE_TERM:
            if (tree->children_count > 1) {
                switch(tree->children[1].node_type) {
                    case UNARYOP_OPTIONAL:
                        tree->regex_nfa = optionalOperationNFA(evaluateRegexTreeRec(&tree->children[0]));
                        break;
                    case UNARYOP_KLEENECLOSURE:
                        tree->regex_nfa = kleeneClosureNFA(evaluateRegexTreeRec(&tree->children[0]));
                        break;
                    case UNARYOP_POSITIVECLOSURE:
                        tree->regex_nfa = positiveClosureNFA(evaluateRegexTreeRec(&tree->children[0]));
                        break;
                    default:
                        fprintf(stderr, "Error in evaluateRegexTreeRec.\n");
                        exit(EXIT_FAILURE);
                }
            }
            else {
                tree->regex_nfa = evaluateRegexTreeRec(&tree->children[0]);
            }
            break;
        case TYPE_FACTOR:
            tree->regex_nfa = evaluateRegexTreeRec(&tree->children[0]);
            break;
        case TYPE_VALUE:
            // Do nothing, just return the NFA.
            break;
        default:
            fprintf(stderr, "Error in evaluateRegexTreeRec.\n");
            exit(EXIT_FAILURE);
    }

    return tree->regex_nfa;
}

// Add a tree to the array, returns the index of the new tree.
unsigned int addTreeToArray (RegexTree *tree_to_add) {
    unsigned int new_tree_index = regex_trees_count;
    regex_trees = realloc(regex_trees, sizeof(RegexTree) * (regex_trees_count+1));
    regex_trees[new_tree_index] = *tree_to_add;
    regex_trees_count++;
    return new_tree_index;
}
