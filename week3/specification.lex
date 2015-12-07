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
expression      (({operand}-{identifier})(\-({operand}-{identifier}))?)
expressions     {expression}(,{expression})*
definition      {openbraces}{expressions}{closebraces}

symbol          [:-?(-.]
quotesymbol     '{symbol}'
operand         {quotechar}|{quotesymbol}|{identifier}|(#{integer})
binaryop        [.|]
unaryop         [?*+]


%%

"section"           { printf ("SECTION_BEGIN ");           }
"end section"       { printf ("SECTION_END ");             }
";"                 { printf ("SEMICOLON ");               }
"options"           { printf ("OPTIONS ");                 }
"lexer"             { printf ("LEXER_OPTION ");            }
"lexeme"            { printf ("LEXEME_OPTION ");           }
"positioning on"    { printf ("POSITIONING_OPTION_ON ");   }
"positioning off"   { printf("POSITIONING_OPTION_OFF ");   }
"where"             { printf ("WHERE_CLAUSE ");            }
"line"              { printf ("POSITIONING_LINE ");        }
"column"            { printf ("POSITIONING_COLUMN ");      }
"default action"    { printf ("DEFAULT_ACTION_OPTION ");   }
"defines"       { printf ("DEFINES ");             }
"define"        { printf ("DEFINE ");              }
"="             { printf ("EQUALS ");              }
"regexps"       { printf ("REGEXPS ");             }
"regexps;"      { return 0;                          } /*aaaaaaaaaaaaaaaa*/
"token"         { printf ("TOKEN_DEF ");           }
"no token"      { printf ("NO_TOKEN_DEF ");        }
"action"        { printf ("ACTION_DEF ");          }
"no action"     { printf ("NO_ACTION_DEF ");       }
"eof"           { printf ("REGEX_EOF ");           }
"anychar"       { printf ("REGEX_ANYCHAR ");       }
"regexp"        { printf ("REGEXP_DEF ");          }
"epsilon"       { printf ("EPSILON ");             }
"("             { printf ("OPEN_PARENTHESIS ");    }
")"             { printf ("CLOSE_PARENTHESIS ");   }
"{"             { printf ("OPEN_CURLYBRACES ");    }
"}"             { printf ("CLOSE_CURLYBRACES ");   }

{identifier}        { printf ("IDENTIFIER ");              }
{definition}    { printf ("DEFINITION ");          }
{operand}       { printf ("OPERAND "); }
{binaryop}      { printf ("BINARYOP "); }
{unaryop}       { printf ("UNARYOP "); }
