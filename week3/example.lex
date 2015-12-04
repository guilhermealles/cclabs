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
    token INPUTTOKEN;
  regexp 'o'.'u'.'t'.'p'.'u'.'t';
    token OUTPUTTOKEN;
  regexp 'w'.'h'.'i'.'l'.'e';
    token  WHILETOKEN;
    action foundWhile;
  regexp 'd'.'o';
    token DOTOKEN;
  regexp 'i'.'f';
    token IFTOKEN;
  regexp 't'.'h'.'e'.'n';
    token THENTOKEN;
  regexp 'e'.'l'.'s'.'e';
    token ELSETOKEN;
  regexp 'e'.'n'.'d';
    token ENDTOKEN;
  regexp ':'.'=';
    token BECOMESTOKEN;
  regexp ';';
    token SEMICOLON;
  regexp #39;
    token QUOTE;
  regexp { '<'; '<'.'='; '<'.'>'; '>'.'='; '>' };
    token RELOP;
  regexp { '+'; '-' };
    token ADDOP;
  regexp { '*'; '*' };
    token MULOP;
  regexp letter.(letter|digit)*;
    token IDENTIFIER;
  regexp digit+.('.'.digit*)?.('e'.('+'|'-'|epsilon).digit+)?;
    token NUMBER;
  regexp eof;
    token EOFTOKEN;
    no action;
  regexp anychar;
    no token;
    action actionAnyChar;
end section regexps;
