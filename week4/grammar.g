{
/**********************************************/
#include <stdio.h>
#include <stdlib.h>
#include "scanner_specification.h"

extern char *yytext;
extern int line;
/**********************************************/
}

%start LLparser, Input;
%token  BEGIN_SECTION_OPTIONS, BEGIN_SECTION_DEFINES, BEGIN_SECTION_REGEXPS, END_SECTION_OPTIONS,
        END_SECTION_DEFINES, END_SECTION_REGEXPS, SEMICOLON, COMMA, LEXER_OPTION, LEXEME_OPTION,
        POSITIONING_OPTION_ON, POSITIONING_OPTION_OFF, WHERE_CLAUSE, POSITIONING_LINE,
        POSITIONING_COLUMN, DEFAULT_ACTION_OPTION, DEFINE, EQUALS,
        TOKEN_DEF, NO_TOKEN_DEF, ACTION_DEF, NO_ACTION_DEF, REGEXP_EOF, REGEXP_ANYCHAR,
        REGEXP_DEF, TOKEN_EPSILON, OPEN_PARENTHESIS, CLOSE_PARENTHESIS, OPEN_CURLYBRACES,
        CLOSE_CURLYBRACES, OPEN_BRACES, CLOSE_BRACES, IDENTIFIER, LITERAL_INT, LITERAL_CHAR,
        RANGE_INT, RANGE_CHAR, OPERAND, BINARYOP, UNARYOP;
%options "generate-lexer-wrapper";
%lexical yylex;


Input                   : SpecificationFile
                        ;

SpecificationFile       :
                            [BEGIN_SECTION_OPTIONS OptionsSection END_SECTION_OPTIONS SEMICOLON]?
                            [BEGIN_SECTION_DEFINES {initializeDefinitionsSection();} DefinesSection END_SECTION_DEFINES SEMICOLON]?
                            [BEGIN_SECTION_REGEXPS {initializeRegexTrees();} RegExpsSection END_SECTION_REGEXPS SEMICOLON {printOptions(); printDefinitions(); exit(EXIT_SUCCESS);}]
                        ;

OptionsSection          :
                            [LEXER_OPTION IDENTIFIER {setLexerRoutine(yytext);} SEMICOLON]?
                            [LEXEME_OPTION IDENTIFIER {setLexemeName(yytext);} SEMICOLON]?
                            [[POSITIONING_OPTION_ON {setPositioningOption(TRUE);} SEMICOLON
                                WHERE_CLAUSE    POSITIONING_LINE IDENTIFIER {setPositioningLineName(yytext);} SEMICOLON
                                                POSITIONING_COLUMN IDENTIFIER {setPositioningColumneName(yytext);} SEMICOLON] |
                             [POSITIONING_OPTION_OFF {setPositioningOption(FALSE);} SEMICOLON]]?
                            [DEFAULT_ACTION_OPTION IDENTIFIER {setDefaultActionRoutineName(yytext);} SEMICOLON]?
                        ;

DefinesSection
{ char *identifier; }        :
                                [DEFINE IDENTIFIER { identifier = malloc(sizeof(char) * strlen(yytext)+1); strcpy(identifier, yytext); } EQUALS OPEN_BRACES
                                    [LITERAL_INT {addLiteralToDefinition(identifier, yytext);}
                                    |LITERAL_CHAR {addLiteralToDefinition(identifier, yytext);}
                                    |RANGE_INT {addRangeToDefinition(identifier, yytext);}
                                    |RANGE_CHAR {addRangeToDefinition(identifier, yytext);}]
                                    [COMMA[LITERAL_INT {addLiteralToDefinition(identifier, yytext);}
                                    |LITERAL_CHAR {addLiteralToDefinition(identifier, yytext);}
                                    |RANGE_INT {addRangeToDefinition(identifier, yytext);}
                                    |RANGE_CHAR {addRangeToDefinition(identifier, yytext);}]]*
                                CLOSE_BRACES SEMICOLON]* {free(identifier);}
                            ;

RegExpsSection          :
                            [REGEXP_DEF [RegularExpressionSet | REGEXP_EOF | REGEXP_ANYCHAR] SEMICOLON { puts("Successfully added one regex!"); }
                             [[TOKEN_DEF IDENTIFIER SEMICOLON] | [NO_TOKEN_DEF SEMICOLON]]
                             [[ACTION_DEF IDENTIFIER SEMICOLON] | [NO_ACTION_DEF SEMICOLON]]?]+
                        ;

RegularExpressionSet
{ RegexTree *r; RegexTree *tree_ptr; }   :
                                            { r = makeNewRegexTree(); tree_ptr = r; } [RegularExpression(tree_ptr)] { addTreeToArray(r); }
                                        |   [OPEN_CURLYBRACES { r = makeNewRegexTree(); tree_ptr = r; } RegularExpression(tree_ptr) { addTreeToArray(r); }
                                            [SEMICOLON { r = makeNewRegexTree(); tree_ptr = r; } RegularExpression(tree_ptr) { addTreeToArray(r); } ]* CLOSE_CURLYBRACES]
                                        ;

RegularExpression(RegexTree *node)
{ RegexTree *child; int operation_type; }   :
                                                { child = regexTreeAddTerm(node); } Term(child)
                                                [BINARYOP { operation_type = parseOperationsToType(yytext); regexTreeAddBinary(node, operation_type); } { child = regexTreeAddTerm(node); } Term(child)]*
                                            ;

Term(RegexTree *node)
{ RegexTree *child; int operation_type; }   :
                                                { child = regexTreeAddFactor(node); } Factor(child) [UNARYOP { operation_type = parseOperationsToType(yytext); regexTreeAddUnary(node, operation_type); }]?
                                            ;

Factor(RegexTree *node)
{ RegexTree *child; }   :
                            [OPERAND | LITERAL_CHAR | LITERAL_INT | IDENTIFIER | TOKEN_EPSILON] { regexTreeAddValue(node, yytext); }
                        |   OPEN_PARENTHESIS { child = regexTreeAddRegex(node);} RegularExpression(child) CLOSE_PARENTHESIS
                        ;


{
/*****************************************************************/
/* the following code is copied verbatim in the generated C file */

void LLmessage(int token) {
    printf("\nSyntax error around \"%s\" on line %d. Aborting...\n", yytext, line);
    exit(-1);
}

int main() {
    LLparser();
    return 0;
}

/*****************************************************************/
}
