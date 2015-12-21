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
    regexp 'i'.'n'.'p'.'u'.'t';
        no token;
        action printFoundInput;
    regexp 'o'.'u'.'t'.'p'.'u'.'t';
        no token;
        action printFoundOutput;
    regexp 'w'.'h'.'i'.'l'.'e';
        no token;
        action printFoundWhile;
    regexp 'd'.'o';
        no token;
        action printFoundDo;
    regexp 'i'.'f';
        no token;
        action printFoundIf;
    regexp 't'.'h'.'e'.'n';
        no token;
        action printFoundThen;
end section regexps;
