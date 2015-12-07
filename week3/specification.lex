%%

white       [ \t]+
digit       [0-9]
integer     {digit}+
letter      [a-z]|[A-Z]
identifier  {letter}+
quoteint    \x27{integer}\x27
quotechar   \x27{letter}\x27

%%

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
