%{  /* The following code is copied verbatim in C file */
#include "misc.h"
#include "ir.tab.h"

static int    linenr = 0;
static int    column = 0;
static char **lines;

void initLexer(char *fnm) {
  int unsigned idx, cnt, line;
  char *text = readFile(fnm);
  for (idx=cnt=0; text[idx] != '\0'; idx++) {
    cnt += (text[idx] == '\n');
  }
  cnt += (text[idx-1] != '\n');
  lines = safeMalloc((1+cnt)*sizeof(char *));
  lines[cnt] = NULL;
  for (line=idx=0; line < cnt; line++) {
    lines[line] = &text[idx];
    while ((text[idx] != '\n') && (text[idx] != '\0')) {
      idx++;      
    }
    text[idx++] = '\0';
  }
  yyin = fopen(fnm, "r");
}

void finalizeLexer() {
  free(lines[0]);
  free(lines);
  fclose(yyin);
}

void showLine(int showcolumn) {
  fprintf(stderr, "%4d: %s\n", linenr+1, lines[linenr]);
  if (showcolumn) {
    int i;
    fprintf(stderr, "      ");
    for (i=0; i < column; i++) {
      fprintf(stderr, " ");
    }
    fprintf(stderr, "^\n");
  }
}

static int symbol(int token) {
  column += strlen(yytext);
  return token;
};

%}

white           [ \t]
digit           [0-9]
letter          [a-zA-Z]
identifier      {letter}({letter}|{digit})*

%option noinput nounput

%% /***************** rules section ***********************/

"="          { return symbol(EQUALS);           }
"+"          { return symbol(PLUS);             }
"-"          { return symbol(MINUS);            }
"*"          { return symbol(TIMES);            }
"/"          { return symbol(DIV);              }
";"          { return symbol(SEMICOLON);        }
{white}      { column++;             /* skip */ }
\n           { linenr++; column = 0; /* skip */ }
{identifier} { return symbol(IDENTIFIER);       }
_{digit}+    { return symbol(IDENTIFIER);       }
{digit}+     { return symbol(INTCONSTANT);      }

.            { showLine(1);
               abortMessage("Unexpected characater [%c]\n", yytext[0]);
             }
