%{
#include <stdio.h>
#include "parser.h"

unsigned int line = 1;
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

%%

"section options"           { return (BEGIN_SECTION_OPTIONS);           }
"section defines"           { return (BEGIN_SECTION_DEFINES);           }
"section regexps"           { return (BEGIN_SECTION_REGEXPS);           }
"end section options"       { return (END_SECTION_OPTIONS);             }
"end section defines"       { return (END_SECTION_DEFINES);             }
"end section regexps"       { return (END_SECTION_REGEXPS);             }
";"                 { return (SEMICOLON);               }
"lexer"             { return (LEXER_OPTION);            }
"lexeme"            { return (LEXEME_OPTION);           }
"positioning on"    { return (POSITIONING_OPTION_ON);   }
"positioning off"   { return (POSITIONING_OPTION_OFF);  }
"where"             { return (WHERE_CLAUSE);            }
"line"              { return (POSITIONING_LINE);        }
"column"            { return (POSITIONING_COLUMN);      }
"default action"    { return (DEFAULT_ACTION_OPTION);   }
"define"            { return (DEFINE);                  }
"="                 { return (EQUALS);                  }
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
"]"                 { return (CLOSE_BRACES);           }
","                 { return (COMMA);                   }
"\n"                { line++;                           }

{identifier}        { return (IDENTIFIER);              }
{quoteint}          { return (LITERAL_INT);             }
{quotechar}         { return (LITERAL_CHAR);            }
{rangeint}          { return (RANGE_INT);               }
{rangechar}         { return (RANGE_CHAR);              }
{operand}           { return (OPERAND);                 }
{binaryop}          { return (BINARYOP);                }
{unaryop}           { return (UNARYOP);                 }
{white}             {}
