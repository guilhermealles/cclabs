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

<<<<<<< HEAD
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
=======
"section"           { return (SECTION_BEGIN);           }
"end section"       { return (SECTION_END);             }
";"                 { return (SEMICOLON);               }
{identifier}        { return (IDENTIFIER);              }

"options"           { return (OPTIONS);                 }
"lexer"             { return (LEXER_OPTION);            }
"lexeme"            { return (LEXEME_OPTION);           }
"positioning"       { return (POSITIONING_OPTION);      }
"on"                { return (POSITIONING_ON);                      }
"off"               { return (POSITIONING_OFF);         }
"where"             { return (WHERE_CLAUSE);            }
"line"              { return (POSITIONING_LINE);        }
"column"            { return (POSITIONING_COLUMN);      }
"default action"    { return (DEFAULT_ACTION_OPTION);   }

"defines"       { return (DEFINES);             }
"define"        { return (DEFINE);              }
"="             { return (EQUALS);              }

"regexps"       { return (REGEXPS);             }
"regexp"        { return (REGEXP_DEF);          }
"token"         { return (TOKEN_DEF);           }
"no token"      { return (NO_TOKEN_DEF);        }
"action"        { return (ACTION_DEF);          }
"no action"     { return (NO_ACTION_DEF);       }
"eof"           { return (REGEX_EOF);           }
"anychar"       { return (REGEX_ANYCHAR);       }


"regexps"       { return (REGEXPS);             }
>>>>>>> origin/master
