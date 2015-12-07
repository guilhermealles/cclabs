%{

#include <stdio.h>

%}

openbraces     \[
closebraces    \]
digit           [0-9]
integer         {digit}+
letter          [a-z]|[A-Z]
identifier      {letter}+
symbol          [:-?(-.]
quoteint        '{integer}'
quotechar       '{letter}'
quotesymbol     '{symbol}'

operand         {quotechar}|{quotesymbol}|{identifier}|(#{integer})
binaryop        [.|]
unaryop         [?*+]



%%

"epsilon"       { printf ("EPSILON ");}
{operand}       { printf ("OPERAND "); }
{binaryop}      { printf ("BINARYOP "); }
{unaryop}       { printf ("UNARYOP "); }
"("             { printf ("OPEN PARENTHESIS ");  }
")"             { printf ("CLOSE PARENTHESIS "); }
