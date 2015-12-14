/* Copyright (C) 2005-2008 G.P. Halkes
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

#ifdef MEM_DEBUG
#include <stdlib.h>

#include "globals.h"
#include "lexer.h"
#include "option.h"
#include "generate.h"
#include "clash.h"

static void freeNonTerminal(NonTerminal *nonTerminal);
static void freeTerm(Term *term);
static void freeAlternative(Alternative *alternative);

/** Free all memory associated with a @a NonTerminal.
	@param nonTerminal The @a NonTerminal which data to free.
*/
static void freeNonTerminal(NonTerminal *nonTerminal) {
	freeTerm(&nonTerminal->term);
	freeToken(nonTerminal->parameters);
	freeToken(nonTerminal->declarations);
	freeToken(nonTerminal->token);
	freeToken(nonTerminal->retvalIdent);
	if (nonTerminal->retvalScope != NULL)
		deleteScope(nonTerminal->retvalScope);
	free(nonTerminal);
}

/** Free all memory associated with a @a Term.
	@param nonTerminal The @a Term which data to free.
*/
static void freeTerm(Term *term) {
	Alternative *alternative;
	
	while ((alternative = listPop(term->rule)) != NULL) {
		freeAlternative(alternative);
		if (!alternative->dontFreeToken)
			freeToken(alternative->token);
		freeToken(alternative->expression);
		free(alternative);
	}
	deleteSet(term->first);
	deleteSet(term->follow);
	deleteSet(term->contains);
	deleteList(term->rule);
}

/** Free all memory associated with a @a Alternative.
	@param nonTerminal The @a Alternative which data to free.
*/
static void freeAlternative(Alternative *alternative) {
	GrammarPart *grammarPart;

	while ((grammarPart = listPop(alternative->parts)) != NULL) {
		switch (grammarPart->subtype) {
			case PART_TERM:
				freeTerm(&grammarPart->uTerm);
				freeToken(grammarPart->uTerm.expression);
				break;
			case PART_ACTION:
				freeToken(grammarPart->uAction);
				break;
			case PART_UNDETERMINED:
			case PART_NONTERMINAL:
				freeToken(grammarPart->uNonTerminal.expression);
				freeToken(grammarPart->uNonTerminal.retvalIdent);
				break;
			default:
				break;
		}
		if (!grammarPart->dontFreeToken)
			freeToken(grammarPart->token);
		free(grammarPart);
	}
	deleteSet(alternative->first);
	deleteSet(alternative->contains);
	deleteList(alternative->parts);
}

/** Free all memory allocated in the program.

	This function is there to allow checking for real memory leaks during the
	execution of the program. In normal operation, free'ing memory at the end
	of the program is a waste of time. valgrind will tell you what parts of
	your malloc'ed memory is unreachable. However, I like to know where all
	the memory is that is still reachable is. By explicitly freeing it, I can
	use valgrind to tell me whether there is any malloc'ed memory left, and
	hence know where it was.
*/
void freeMemory(void) {
	Declaration *declaration;
	char *name;

	cleanUpLexer();
	
	while ((declaration = listPop(declarations)) != NULL) {
		switch (declaration->subtype) {
			case CODE:
				freeToken(declaration->uCode);
				break;
			case DIRECTIVE:
				/* Only free tokens for directives named in text */
				if (!declaration->uDirective->dontFreeTokens) {
					freeToken(declaration->uDirective->token[0]);
					/* Don't free label set for token. */
					if (declaration->uDirective->subtype != TOKEN_DIRECTIVE)
						freeToken(declaration->uDirective->token[1]);
				}
				free(declaration->uDirective);
				break;
			case NONTERMINAL:
				freeNonTerminal(declaration->uNonTerminal);
				break;
			default:
				break;
		}
		free(declaration);
	}
	deleteList(declarations);

	deleteListWithContents(option.inputFileList);
	
	deleteList(inputStack);

	declaration = lookup(globalScope, "EOFILE");

	freeToken(declaration->uDirective->token[0]);
	free(declaration->uDirective);
	free(declaration);
	
	deleteScope(globalScope);
	
	free(terminalToCondensed);
	free(condensedToTerminal);
	
	if (option.outputBaseNameSet) {
		freeToken(option.outputBaseNameLocation);
		free((char *) option.outputBaseName);
	}

	if (option.extensions != NULL) {
		freeToken(option.extensionsLocation);
		deleteListWithContents(option.extensions);
	}

	if (option.unusedIdentifiers != NULL) {
		while ((name = listPop(option.unusedIdentifiers)) != NULL)
			free(name);
		deleteList(option.unusedIdentifiers);
	}
	free((char *) option.dependTargets);
	free((char *) option.dependExtraTargets);
	free((char *) option.dependFile);
	
	free(setList);

	/* Note: dataTypeDirective does NOT point to valid memory anymore! */
	if (dataTypeDirective != NULL) {
		free((char *) userDataTypeTS);
		free((char *) userDataTypeHeaderTS);
	}
	
	freeClashScope();
}

void freeClashScopeStrings(const char *key, void *data, UNUSED void *userData) {
	if (data == (void *) 2)
		free((char *) key);
}

#else
/* Don't do anything. It takes time, which is not necessary as the OS will
   clean up the memory used anyway. */
void freeMemory(void) { }
#endif
