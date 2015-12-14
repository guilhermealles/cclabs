/* Copyright (C) 2005-2008,2011 G.P. Halkes
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

#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include "list.h"
#include "bool.h"
#include "scope.h"

typedef struct {
	char *text;
	const char *fileName;
	int lineNumber;
} Token;

/* NOTE: if CCode is changed to something other than a token, several pieces of
   code need to be changed as well. For example, copyCCode and calls to
   freeToken. */
typedef Token CCode;

Token *newToken(void);
Token *copyToken(Token *original, const char *file, int line);
CCode *newCCode(void);
CCode *copyCCode(CCode *original, const char *file, int line);
Token *newPlaceHolder(void);
void freeToken(Token *token);

typedef enum {
	START_DIRECTIVE = 1,
	TOKEN_DIRECTIVE,
	FIRST_DIRECTIVE,
	LEXICAL_DIRECTIVE,
	PREFIX_DIRECTIVE,
	ONERROR_DIRECTIVE,
	LABEL_DIRECTIVE,
	DATATYPE_DIRECTIVE,
	LAST_DIRECTIVE_ENTRY /* Used for directiveToText */
} DirectiveSubtype;

typedef struct Directive {
	DirectiveSubtype subtype;
	Token *token[2];			/* For everything with one or two tokens */
	union {
		struct Directive *next;	/* For checking %start and %first directives */
		int number;				/* Terminal number */
	} un;
#ifdef MEM_DEBUG
	bool dontFreeTokens;
#endif
} Directive;

Directive *newDirective(DirectiveSubtype subtype, Token *token1, Token *token2);

typedef enum {
	CODE = 1,
	DIRECTIVE,
	NONTERMINAL
} DeclarationSubtype;

#include "ruleAnalysis.h"

typedef struct {
	DeclarationSubtype subtype;
	bool valid;
	union {
		CCode *code;
		Directive *directive;
		NonTerminal *nonTerminal;
		void *ptr;	/* To make newDeclaration easier */
	} un;
} Declaration;

Declaration *newDeclaration(DeclarationSubtype subtype, void *ptr);

extern int maxTokenNumber;
extern Token *literalLabels[256];

#define CONDENSED_ISTOKEN (1<<0)
#define CONDENSED_REACHABLE (1<<1)

typedef struct {
	int flags;
	union {
		Directive *token;
		int literal;
	} un;
} CondensedInfo;

extern List *declarations; /* List of all declarations */
extern Scope *globalScope;

extern Directive *lexicalDirective;
extern Directive *prefixDirective;
extern const char *prefix;
extern Directive *onErrorDirective;
extern Directive *dataTypeDirective;
extern CCode *top_code;

extern int *terminalToCondensed;
extern CondensedInfo *condensedToTerminal;
extern int condensedNumber;
extern int nrOfTerminals;

extern bool hasFirst;

void setupScope(void);
void setupResolve(void);
void checkDeclarations(void);
void fixCondensedTable(void);
void walkDirectives(void (*action)(Directive *));
void enumerateMacroSets(Directive *directive);
int checkLiteral(Token *literal);

#endif
