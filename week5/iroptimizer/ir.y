/****** options section ********/

%{  /* copied verbatim in C file */
    #include <stdio.h>
    #include <stdlib.h>
    #include "lex.yy.c"
    #include "quadruple.h"
    
    extern void processQuadruple(quadruple q);
    
    /* some global variables, but statically declared (so safe) */
    char *lhs, *operand1, *operand2;
    operator op;
    
    int yyerror(const char *s) {
        showLine(1);
        printf("%s\n",s);
        exit(EXIT_FAILURE);
    }
    
    %}

/* Bison declarations.  */
/* %define parse.error verbose */
%token IDENTIFIER EQUALS INTCONSTANT SEMICOLON
%token PLUS MINUS TIMES DIV

%start IRgrammar


%%  /****** grammar rules section ********/

IRgrammar : Line IRgrammar
| /* empty */
;

Line      : Lhs EQUALS Rhs
{ quadruple q = makeQuadruple(lhs, op, operand1, operand2);
    processQuadruple(q);
    free(lhs);
    free(operand1);
    free(operand2);
}
SEMICOLON
;

Lhs       : IDENTIFIER { lhs = stringDuplicate(yytext); }
;

Rhs       : Operand1 { op = ASSIGNMENT; operand2 = NULL; }
| Operand1 Operator Operand2
;

Operand1  : IDENTIFIER  { operand1 = stringDuplicate(yytext); }
| INTCONSTANT { operand1 = stringDuplicate(yytext); }
;

Operand2  : IDENTIFIER  { operand2 = stringDuplicate(yytext); }
| INTCONSTANT { operand2 = stringDuplicate(yytext); }
;

Operator  : PLUS  { op = PLUSOP;  }
| MINUS { op = MINUSOP; }
| TIMES { op = TIMESOP; }
| DIV   { op = DIVOP;   }
;
