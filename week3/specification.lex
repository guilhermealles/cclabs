%{
#include <stdio.h>
%}

white           [ \t]+
digit           [0-9]
integer         {digit}+
letter          [a-z]|[A-Z]
identifier      {letter}+
openbraces      \[
closebraces     \]
quoteint        '{integer}'
quotechar       '{letter}'
rangeint        {quoteint}\-{quoteint}
rangechar       {quotechar}\-{quotechar}

expression      ({quoteint}(\-{quoteint})?)|({quotechar}(\-{quotechar})?)
expressions     {expression}(,{expression})*

symbol          [:-?(-.]
quotesymbol     '{symbol}'
operand         {quotechar}|{quotesymbol}|{identifier}|(#{integer})
binaryop        [.|]
unaryop         [?*+]

<<<<<<< HEAD
white               [ \t]+
digit               [0-9]
integer             {digit}+
letter              [a-z]|[A-Z]
identifier          {letter}+
quoteint            \x27{integer}\x27
quotechar           \x27{letter}\x27
open_parenthesis    \(
close_parenthesis   \)
operator            [\+ | \- | \* | \< | \> ]
binary_operator     [\< | \>][\= | \>]
optional_operator   [\?]
epsilon             epsilon

=======
>>>>>>> origin/master

%%

"section"           { return (SECTION_BEGIN);           }
"end section"       { return (SECTION_END);             }
";"                 { return (SEMICOLON);               }
"options"           { return (OPTIONS);                 }
"lexer"             { return (LEXER_OPTION);            }
"lexeme"            { return (LEXEME_OPTION);           }
"positioning on"    { return (POSITIONING_OPTION_ON);   }
"positioning off"   { return (POSITIONING_OPTION_OFF);  }
"where"             { return (WHERE_CLAUSE);            }
"line"              { return (POSITIONING_LINE);        }
"column"            { return (POSITIONING_COLUMN);      }
"default action"    { return (DEFAULT_ACTION_OPTION);   }
"defines"           { return (DEFINES);                 }
"define"            { return (DEFINE);                  }
"="                 { return (EQUALS);                  }
"regexps"           { return (REGEXPS);                 }
"token"             { return (TOKEN_DEF);               }
"no token"          { return (NO_TOKEN_DEF);            }
"action"            { return (ACTION_DEF);              }
"no action"         { return (NO_ACTION_DEF);           }
"eof"               { return (REGEXP_EOF);               }
"anychar"           { return (REGEXP_ANYCHAR);           }
"regexp"            { return (REGEXP_DEF);              }
"epsilon"           { return (EPSILON);                 }
"("                 { return (OPEN_PARENTHESIS);        }
")"                 { return (CLOSE_PARENTHESIS);       }
"{"                 { return (OPEN_CURLYBRACES);        }
"}"                 { return (CLOSE_CURLYBRACES);       }
"["                 { return (OPEN_BRACES);             }
"]"                 { return (CLOSE_BRACES");           }

{identifier}        { return (IDENTIFIER);              }
{quoteint}          { return (LITERAL_INT);             }
{quotechar}         { return (LITERAL_CHAR);            }
{rangeint}          { return (RANGE_INT);               }
{rangechar}         { return (RANGE_CHAR);              }
{operand}           { return (OPERAND);                 }
{binaryop}          { return (BINARYOP);                }
{unaryop}           { return (UNARYOP);                 }
