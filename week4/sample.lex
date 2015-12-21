section options
  lexer yylex;
  lexeme yytext;
  positioning on;
    where line yylineno;
          column yycolumn;
  default action defaultAction;
end section options;

section defines
  define digit  = ['0'-'9'];
  define letter = ['a','b','c'-'z','A'-'Y', 'Z'];
end section defines;

section regexps
    regexp { '<'; '<'.'='; '<'.'>'; '>'.'='; '>' };
        token RELOP;
    regexp 'd'.'o';
        token DOTOKEN;
    regexp 'i'.'f';
        token IFTOKEN;
    regexp 't'.'h'.'e'.'n';
        token THENTOKEN;
    regexp 'e'.'l'.'s'.'e';
        token ELSETOKEN;
end section regexps;
