/* Copyright (C) 2005,2006,2008 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
 
#ifndef LEXER_H
#define LEXER_H

#include "bool.h"

/* Current line in the input */
extern int lineNumber;
/* codeStart is the line number of the first line of a code segment. lineNumber
   cannot be used because it may be multiline. */
extern int codeStart;
/* Current input file name */
extern const char *fileName;

extern char *yytext;
extern FILE *yyin;
/* stack of input files used by the lexer */
extern List *inputStack;

extern List *includedFiles;

void openFirstInput(void);
bool nextFile(void);

/* Function defined in the lexer */
bool openInclude(Token *file);

#ifdef MEM_DEBUG
void cleanUpLexer(void);
#endif
#endif
