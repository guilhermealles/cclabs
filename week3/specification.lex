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

%%

<<<<<<< Updated upstream
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
=======
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
"defines"           { printf ("DEFINES ");                  }
"define"            { printf ("DEFINE ");                   }
"="                 { printf ("EQUALS ");                   }
"regexps"           { printf ("REGEXPS ");                  }
"regexps;"          { return 0;                             } /*aaaaaaaaaaaaaaaa*/
"token"             { printf ("TOKEN_DEF ");                }
"no token"          { printf ("NO_TOKEN_DEF ");             }
"action"            { printf ("ACTION_DEF ");               }
"no action"         { printf ("NO_ACTION_DEF ");            }
"eof"               { printf ("REGEX_EOF ");                }
"anychar"           { printf ("REGEX_ANYCHAR ");            }
"regexp"            { printf ("REGEXP_DEF ");               }
"epsilon"           { printf ("EPSILON ");                  }
"("                 { printf ("OPEN_PARENTHESIS ");         }
")"                 { printf ("CLOSE_PARENTHESIS ");        }
"{"                 { printf ("OPEN_CURLYBRACES ");         }
"}"                 { printf ("CLOSE_CURLYBRACES ");         }

{identifier}        { printf ("IDENTIFIER ");              }
{definition}        { printf ("DEFINITION ");          }
{operand}           { printf ("OPERAND "); }
{binaryop}          { printf ("BINARYOP "); }
{unaryop}           { printf ("UNARYOP "); }
>>>>>>> Stashed changes
