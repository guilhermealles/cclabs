{
/**********************************************/
#include <stdio.h>
#include <stdlib.h>

extern char *yytext;
/**********************************************/
}

%start LLparser Input;
%token  SECTION_BEGIN, SECTION_END, SEMICOLON, COMMA, OPTIONS, LEXER_OPTION, LEXEME_OPTION,
        POSITIONING_OPTION_ON, POSITIONING_OPTION_OFF, WHERE_CLAUSE, POSITIONING_LINE,
        POSITIONING_COLUMN, DEFAULT_ACTION_OPTION, DEFINES, DEFINE, EQUALS, REGEXPS,
        TOKEN_DEF, NO_TOKEN_DEF, ACTION_DEF, NO_ACTION_DEF, REGEXP_EOF, REGEXP_ANYCHAR,
        REGEXP_DEF, EPSILON, OPEN_PARENTHESIS, CLOSE_PARENTHESIS, OPEN_CURLYBRACES,
        CLOSE_CURLYBRACES, OPEN_BRACES, CLOSE_BRACES, IDENTIFIER, LITERAL_INT, LITERAL_CHAR,
        RANGE_INT, RANGE_CHAR, OPERAND, BINARYOP, UNARYOP;
%options "generate-lexer-wrapper";
%lexical yylex


Input                   : SpecificationFile
                        ;

SpecificationFile       :
                            [SECTION_BEGIN OPTIONS OptionsSection SECTION_END OPTIONS SEMICOLON]?
                            [SECTION_BEGIN DEFINES DefinesSection SECTION_END DEFINES SEMICOLON]?
                            [SECTION_BEGIN REGEXPS RegExpsSection SECTION_END REGEXPS SEMICOLON]
                        ;

OptionsSection          :
                            [LEXER_OPTION IDENTIFIER SEMICOLON]?
                            [LEXEME_OPTION IDENTIFIER SEMICOLON]?
                            [[POSITIONING_OPTION_ON SEMICOLON
                                WHERE_CLAUSE    POSITIONING_LINE IDENTIFIER SEMICOLON
                                                POSITIONING_COLUMN IDENTIFIER SEMICOLON] |
                             [POSITIONING_OPTION_OFF SEMICOLON]]?
                            [DEFAULT_ACTION_OPTION IDENTIFIER SEMICOLON]?
                        ;

DefinesSection          :
                            [DEFINE IDENTIFIER EQUALS OPEN_BRACES
                                [LITERAL_INT|LITERAL_CHAR|RANGE_INT|RANGE_CHAR][COMMA[LITERAL_INT|LITERAL_CHAR|RANGE_INT|RANGE_CHAR]]*
                            CLOSE_BRACES SEMICOLON]*
                        ;

RegExpsSection          :
                            [REGEXP_DEF [RegularExpressionSet | REGEXP_EOF | REGEXP_ANYCHAR]
                             [[TOKEN_DEF IDENTIFIER] | [NO_TOKEN_DEF]]
                             [[ACTION_DEF IDENTIFIER] | [NO_ACTION_DEF]]?]+
                        ;

RegularExpressionSet    :
                            [RegularExpression SEMICOLON] | [OPEN_CURLYBRACES RegularExpression [SEMICOLON RegularExpression]* CLOSE_CURLYBRACES] SEMICOLON
                        ;

RegularExpression       :
                            [Term] [BINARYOP Term]* [UNARYOP]?
                        ;


Term                    :
                            [[OPERAND][UNARYOP]?] | [OPEN_PARENTHESIS Term CLOSE_PARENTHESIS]
                        ;
