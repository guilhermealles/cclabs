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

#include <stdlib.h>
#include <string.h>

#include "nonRuleAnalysis.h"
#include "globals.h"
#include "option.h"
#include "lexer.h"
#include "clash.h"

#include "posixregex.h"

List *declarations = NULL;
Scope *globalScope;

Directive *lexicalDirective = NULL;
Directive *prefixDirective = NULL;
const char *prefix = "LL";
Directive *onErrorDirective = NULL;
Directive *dataTypeDirective = NULL;
CCode *top_code = NULL;

int *terminalToCondensed;
CondensedInfo *condensedToTerminal;
int condensedNumber = 0;
int nrOfTerminals = 0;

bool hasFirst = false;

/** Allocate and initialise a new @a Token.
	@return A newly allocated and initialised @a Token.

	The @a Token is initialised with the current line number and file name,
	and a copy of yytext.

	The function does not return if memory could not be allocated.
*/
Token *newToken(void) {
	Token *retval;

	retval = (Token *) safeMalloc(sizeof(Token), "newToken");
	retval->lineNumber = lineNumber;
	retval->fileName = fileName;
	retval->text = safeStrdup(yytext, "newToken");

	return retval;
}

/** Allocate and copy a @a Token struct.
	@param token The @a Token to be copied.
	@param file The file name to be used or NULL for copy.
	@param line The line number to be used or <0 for copy.
*/
Token *copyToken(Token *original, const char *file, int line) {
	Token *copy;

	if (original == NULL)
		return NULL;

	copy = (Token *) safeMalloc(sizeof(Token), "copyToken");
	memcpy(copy, original, sizeof(Token));
	copy->text = safeStrdup(original->text, "copyToken");

	if (fileName != NULL)
		copy->fileName = file;

	if (lineNumber >= 0)
		copy->lineNumber = line;

	return copy;
}

/** Allocate and initialise a new @a CCode struct.
	@return A newly allocated and initialised @a CCode struct.

	The @a CCode is initialised with the current line number and file name,
	and a copy of yytext.

	The function does not return if memory could not be allocated.
*/
CCode *newCCode(void) {
	CCode *retval;

	retval = (CCode *) safeMalloc(sizeof(CCode), "newCCode");
	retval->lineNumber = codeStart;
	retval->fileName = fileName;
	retval->text = safeStrdup(yytext, "newCCode");

	return retval;
}

/** Allocate and copy a @a CCode struct.
	@param code The @a CCode to be copied.
	@param file The file name to be used or NULL for copy.
	@param line The line number to be used or <0 for copy.
*/
CCode *copyCCode(CCode *original, const char *file, int line) {
	return (CCode *) copyToken(original, file, line);
}

/** Allocate a place-holder @a Token.

	A place-holder @a Token only contains a line number and file name. The
	@a text member is NULL.
*/
Token *newPlaceHolder(void) {
	Token *retval;

	retval = (Token *) safeMalloc(sizeof(Token), "newPlaceHolder");
	retval->lineNumber = lineNumber;
	retval->fileName = fileName;
	retval->text = NULL;

	return retval;
}

/** Delete a @a Token.
	@param token The @a Token to be deleted.

	This function free's both the internal data as well as the struct itself.
*/
void freeToken(Token *token) {
	if (token == NULL)
		return;
	free(token->text);
	free(token);
}

/** Allocate and initialise a new @a Directive.
	@param subtype The subtype of the new @a Directive.
	@param token1 The first token associated with the @a Directive.
	@param token1 The second token associated with the @a Directive.
	@return A newly allocated and initialised @a Directive.

	The function does not return if memory could not be allocated.
*/
Directive *newDirective(DirectiveSubtype subtype, Token *token1, Token *token2) {
	Directive *retval;

	retval = (Directive *) safeCalloc(sizeof(Directive), "newDirective");
	retval->subtype = subtype;
	retval->token[0] = token1;
	retval->token[1] = token2;

	return retval;
}

/** Delete a @a Directive and it's tokens, if available. */
void deleteDirective(Directive *directive) {
	if (directive->token[0] != NULL)
		freeToken(directive->token[0]);
	if (directive->token[1] != NULL)
		freeToken(directive->token[1]);
	free(directive);
}

/** Allocate and initialise a new @a Declaration.
	@param subtype The subtype of the new @a Declaration.
	@param ptr The data representing the @a Declaration.
	@return A newly allocated and initialised @a Declaration.

	The function does not return if memory could not be allocated.
*/
Declaration *newDeclaration(DeclarationSubtype subtype, void *ptr) {
	Declaration *retval;

	retval = (Declaration *) safeMalloc(sizeof(Declaration), "newDeclaration");
	retval->subtype = subtype;
	retval->valid = true;
	retval->uPtr = ptr;

	listAppend(declarations, retval);

	return retval;
}

/** Delete a @a Declaration. */
void deleteDeclaration(Declaration *declaration) {
	free(declaration);
}

/* Everything under 257 has been reserved for the characters they represent
   and the EOFILE token */
int maxTokenNumber = 257;

/** Check that a symbol is the one declared in the global scope, or print an error message.
	@param declaration The declaration associated with the symbol.
	@param token The @a Token representing the symbol.
*/
static void checkSymbolWithScope(Declaration *declaration, Token *token) {
	Declaration *existing;
	if ((existing = lookup(globalScope, token->text)) != declaration) {
		error(token, "Symbol '%s' already declared as a ", token->text);
		if (existing->subtype == DIRECTIVE && existing->uDirective->subtype == TOKEN_DIRECTIVE) {
			continueMessage("token at ");
			printAt(existing->uDirective->token[0]);
		} else if (existing->subtype == NONTERMINAL) {
			continueMessage("non-terminal at ");
			printAt(existing->uNonTerminal->token);
		} else {
			PANIC();
		}
		endMessage();
	}
	if (checkClash(token->text, SEL_LLRETVAL))
		softError(token, "Symbol '%s' clashes with generated symbol\n", token->text);

}

/* This array is memset to 0 in setupScope */
Token *literalLabels[256];
/* This array is initialised in setupScope */
static const char *directiveToText[LAST_DIRECTIVE_ENTRY];
static Scope *directiveScope;

/** Check the arguments of a %label directive for validity.
	@param directive The %label directive.

	This function checks:
	@li whether %label references a valid token.
	@li the validity of charachter literals.
	@li double %label's.
*/
static void checkLabel(Directive *directive) {
	Token *firstToken = directive->token[0];

	if (firstToken->text[0] == '\'') {
		int literal = checkLiteral(firstToken);
		if (literal < 0)
			return;
		if (literalLabels[literal] != NULL) {
			softError(firstToken, "Literal %s is already labeled at ", firstToken->text);
			printAt(literalLabels[literal]);
			endMessage();
			return;
		}
		literalLabels[literal] = directive->token[1];
		return;
	} else {
		Declaration *existing;
		existing = (Declaration *) lookup(globalScope, firstToken->text);
		if (existing == NULL) {
			ASSERT(option.noAllowLabelCreate);
			softError(firstToken, "%%label used for non-defined token '%s'\n", firstToken->text);
		} else {
			if (existing->subtype == DIRECTIVE) {
				/* Only token directives are added to the globalScope. */
				ASSERT(existing->uDirective->subtype == TOKEN_DIRECTIVE);
				if (existing->uDirective->token[1] != NULL) {
					softError(firstToken, "Token '%s' is already labeled at ", firstToken->text);
					printAt(existing->uDirective->token[1]);
					endMessage();
					return;
				}
				/* The declaration in the globalScope is the same as the one in
				   the declarations list, therefore this will have the desired
				   effect. */
				existing->uDirective->token[1] = directive->token[1];
			} else if (existing->subtype == NONTERMINAL) {
				softError(firstToken, "%%label used for non-terminal '%s'\n", firstToken->text);
			} else {
				PANIC();
			}
			return;
		}
	}
	return;
}

/** Routine to check for existing directives, with appropriate errors and
		warnings.
	@param directive The @a Directive to check.
*/
static void checkDirectiveDeclaration(Directive *directive) {
	static Directive *list = NULL;
	Directive *next = list;
	Directive *existing;
	Declaration *declaration;

	existing = (Directive *) lookup(directiveScope, directive->token[0]->text);

	if (existing != NULL) {
		softError(directive->token[0], "A %s directive with name '%s' has already been defined at ",
			directiveToText[existing->subtype], directive->token[0]->text);
		printAt(existing->token[0]);
		endMessage();
		return;
	}
	/* We need to do a check the directives, to make sure they don't clash
	   with any %token directives. This would give strange compiler messages
	   if we don't. */
	declaration = (Declaration *) lookup(globalScope, directive->token[0]->text);
	if (declaration != NULL && declaration->subtype == DIRECTIVE && declaration->uDirective->subtype == TOKEN_DIRECTIVE) {
		softError(directive->token[0], "A token with name '%s' has already been defined at ",
			directive->token[0]->text);
		printAt(declaration->uDirective->token[0]);
		endMessage();
		return;
	}

	if (checkClash(directive->token[0]->text, SEL_LLRETVAL))
		softError(directive->token[0], "%s directive with name '%s' clashes with generated name\n", directiveToText[directive->subtype], directive->token[0]->text);

	declare(directiveScope, directive->token[0]->text, directive);

	if (directive->subtype != START_DIRECTIVE && directive->subtype != FIRST_DIRECTIVE)
		return;
	/* The code below here is only for %start and %first directives, so we have
	   the check above to ensure that. */

	if (!(declaration = (Declaration *) lookup(globalScope, directive->token[1]->text))) {
		softError(directive->token[1], "%s directive specifies non-existant non-terminal '%s'\n", directiveToText[directive->subtype], directive->token[1]->text);
	} else if (declaration->subtype == DIRECTIVE && declaration->uDirective->subtype == TOKEN_DIRECTIVE) {
		softError(directive->token[1], "%s directive specifies token '%s' instead of a non-terminal\n", directiveToText[directive->subtype], directive->token[1]->text);
	} else if (declaration->subtype == NONTERMINAL) {
		if (directive->subtype == START_DIRECTIVE) {
			/* This non-terminal can be reached, and can end on EOF. We don't
			   set the EOFILE token in the follow set here, as the sets have
			   not yet been allocated. The allocateSets function will also set
			   this token. */
			declaration->uNonTerminal->flags |= NONTERMINAL_START;
			if (option.LLgenStyleOutputs)
				declaration->uNonTerminal->flags |= NONTERMINAL_GLOBAL;
		} else {
			/* This non-terminal is mentioned in a %first, so suppress
			   warnings about unused non-terminals. */
			declaration->uNonTerminal->flags |= NONTERMINAL_FIRST;
		}
	} else {
		PANIC();
	}

	/* Check for duplicate %start and %first directives. */
	if (list == NULL) {
		list = directive;
		return;
	}
	while (1) {
		if (next->subtype == directive->subtype && strcmp(next->token[1]->text, directive->token[1]->text) == 0) {
			/* Check here is necessary as the printAt and endMessage functions
			   don't have a check and would print otherwise */
			if (!option.suppressWarnings) {
				warning(WARNING_UNMASKED, directive->token[1], "A %s directive for the symbol '%s' has already been defined at ",
					directiveToText[next->subtype], directive->token[1]->text);
				printAt(next->token[1]);
				endMessage();
			}
		}
		if (next->uNext == NULL)
			break;
		next = next->uNext;
	}
	next->uNext = directive;
}

/** Fill the global scope with the symbols (tokens and non-terminals).

	This function allocates and fills the scope, and checks for inconsistent
	declarations.
*/
void setupScope(void) {
	int i;
	Declaration *declaration;
	Directive *directive;
	Token *eofileToken;

	/* Initialise variables used in filling the scopes */
	memset(literalLabels, 0, sizeof(Token *) * 256);
	directiveToText[START_DIRECTIVE] = "%start";
	directiveToText[TOKEN_DIRECTIVE] = "%token";
	directiveToText[FIRST_DIRECTIVE] = "%first";
	directiveToText[LEXICAL_DIRECTIVE] = "%lexical";
	directiveToText[PREFIX_DIRECTIVE] = "%prefix";
	directiveToText[ONERROR_DIRECTIVE] = "%onerror";
	directiveToText[LABEL_DIRECTIVE] = "%label";
	directiveToText[DATATYPE_DIRECTIVE] = "%datatype";

	globalScope = newScope();
	directiveScope = newScope();

	/* Create the EOFILE token. This needs to be done before the handling of
	   the LABEL_DIRECTIVE's, as they may rename it. */
	eofileToken = (Token *) safeCalloc(sizeof(Token), "setupScope");
	/* Use strdup to make sure we don't segfault on changing the text (and
	   to shut up the compiler). */
	eofileToken->text = safeStrdup("EOFILE", "setupScope");

	directive = newDirective(TOKEN_DIRECTIVE, eofileToken, NULL);
	directive->uNumber = 256;

	declaration = (Declaration *) safeMalloc(sizeof(Declaration), "newDeclaration");
	declaration->subtype = DIRECTIVE;
	declaration->uPtr = directive;

	declare(globalScope, eofileToken->text, declaration);

	/* Fill the global scope with the %token defined tokens. Don't care about
	   duplicates, they will be detected later. This is to allow any order of
	   %label and %token directives, even with --no-allow-label-create. */
	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == DIRECTIVE) {
			directive = declaration->uDirective;
			switch (directive->subtype) {
				case TOKEN_DIRECTIVE:
					if (declare(globalScope, directive->token[0]->text, declaration))
						directive->uNumber = maxTokenNumber++;
					else
						declaration->valid = false;
					break;
				case PREFIX_DIRECTIVE:
					/* Register prefixDirective here, so that we can generate proper
					   warnings when checking the symbols. */
					if (prefixDirective == NULL) {
						prefixDirective = directive;
						prefix = prefixDirective->token[0]->text;
					}
					break;
				default:
					break;
			}
		} else if (declaration->subtype == NONTERMINAL) {
			declare(globalScope, declaration->uNonTerminal->token->text, declaration);
		}
	}

	/* Add the tokens created by labels to the global scope. We don't add the
	   labels here yet, because it makes checking easier if the pointer is set
	   to NULL. */
	if (!option.noAllowLabelCreate) {
		int declarationsSize = listSize(declarations);
		for (i = 0; i < declarationsSize; i++) {
			declaration = (Declaration *) listIndex(declarations, i);
			if (declaration->subtype == DIRECTIVE) {
				directive = declaration->uDirective;
				if (directive->subtype == LABEL_DIRECTIVE && directive->token[0]->text[0] != '\'' && lookup(globalScope, directive->token[0]->text) == NULL) {
					/* This can be done in a nested call without the local variables,
					   but this way is cleaner */
					Declaration *tokenDeclaration;
					Directive *tokenDirective;

					/* Create and declare a new Token from label */
					tokenDirective = newDirective(TOKEN_DIRECTIVE, directive->token[0], NULL);
					tokenDirective->uNumber = maxTokenNumber++;
					tokenDeclaration = newDeclaration(DIRECTIVE, tokenDirective);
#ifdef MEM_DEBUG
					tokenDirective->dontFreeTokens = true;
#endif

					declare(globalScope, directive->token[0]->text, tokenDeclaration);
				}
			}
		}
	}

#ifdef USE_REGEX
	/* Add identifiers that match the user-supplied pattern, and do not yet
	   appear in the global scope to the global scope. */
	if (option.useTokenPattern)
		scanForUnidentifiedTokens();
#endif

	initialiseClashScope();
}

/** Sets up the name resolution process, including the terminal translation tables. */
void setupResolve(void) {
	int i;
	Declaration *declaration;

	terminalToCondensed = (int *) safeMalloc(maxTokenNumber * sizeof(int), "setupResolve");

	memset(terminalToCondensed, -1, maxTokenNumber * sizeof(int));

	terminalToCondensed[256] = condensedNumber++;
	if (!option.noEOFZero)
		terminalToCondensed[0] = terminalToCondensed[256];

	condensedToTerminal = (CondensedInfo *) safeCalloc((maxTokenNumber - 256) * sizeof(CondensedInfo), "setupResolve");
	/* The following is implicit through safeCalloc (i.e. EOF/nul is a literal)
	condensedToTerminal[0].flags = 0;
	condensedToTerminal[0].uLiteral = 0;
	*/

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		/* Skip invalid declarations. */
		if (!declaration->valid)
			continue;
		if (declaration->subtype == DIRECTIVE &&
				declaration->uDirective->subtype == TOKEN_DIRECTIVE) {
			terminalToCondensed[declaration->uDirective->uNumber] = condensedNumber;
			condensedToTerminal[condensedNumber].flags |= CONDENSED_ISTOKEN;
			condensedToTerminal[condensedNumber].uToken = declaration->uDirective;
			condensedNumber++;
		}
	}
}

void checkDeclarations(void) {
	bool hasStart = false;
	bool multipleStartWarned = false;
	bool firstPrefix = true;
	Declaration *declaration;
	Directive *directive;
	int i;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		/* Check the directives. */
		if (declaration->subtype == DIRECTIVE) {
			directive = declaration->uDirective;
			switch (directive->subtype) {
				case START_DIRECTIVE:
					checkDirectiveDeclaration(directive);
					if (hasStart && !multipleStartWarned && !option.reentrant && !option.threadSafe) {
						warning(WARNING_MULTIPLE_PARSER, directive->token[0], "Multiple parsers in one grammar can require --reentrant or --thread-safe.\n\tSee documentation for details.\n");
						multipleStartWarned = true;
					}
					hasStart = true;
					break;
				case TOKEN_DIRECTIVE:
					checkSymbolWithScope(declaration, directive->token[0]);
					break;
				case LABEL_DIRECTIVE:
					checkLabel(directive);
					break;
				case FIRST_DIRECTIVE:
					checkDirectiveDeclaration(directive);
					break;
				case LEXICAL_DIRECTIVE:
					checkDirectiveDeclaration(directive);
					lexicalDirective = directive;
					break;
				case PREFIX_DIRECTIVE:
					/* The prefixDirective and prefix variables have been set in setupScope.
					   However, the error is generated here to keep the errors in order. */
					if (firstPrefix) {
						firstPrefix = false;
					} else {
						softError(directive->token[0], "%%prefix already specified at ");
						printAt(prefixDirective->token[0]);
						endMessage();
					}
					break;
				case ONERROR_DIRECTIVE:
					if (onErrorDirective == NULL) {
						onErrorDirective = directive;
					} else {
						softError(directive->token[0], "%%onerror already specified at ");
						printAt(onErrorDirective->token[0]);
						endMessage();
					}
					break;
				case DATATYPE_DIRECTIVE:
					if (dataTypeDirective == NULL) {
						dataTypeDirective = directive;
						if (!option.threadSafe)
							warning(WARNING_DATATYPE, NULL, "%%datatype used without --thread-safe\n");
					} else {
						softError(directive->token[0], "%%datatype already specified at ");
						printAt(dataTypeDirective->token[0]);
						endMessage();
					}
					break;
				default:
					PANIC();
			}
		} else if (declaration->subtype == NONTERMINAL) {
			/* Check the names of the non-terminals. */
			checkSymbolWithScope(declaration, declaration->uNonTerminal->token);
			resolve(declaration->uNonTerminal);
		} else if (declaration->subtype != CODE) { /* Should be only C code */
			PANIC();
		}
	}
	if (!hasStart)
		softError(0, "No %%start directive was specified\n");

	/* The directiveScope is no longer necessary after here. */
	deleteScope(directiveScope);
}

/** Adds the literals to the condensed table */
void fixCondensedTable(void) {
	int i;
	/* Note: the newly allocated memory is uninitialised! */
	condensedToTerminal = (CondensedInfo *) safeRealloc(condensedToTerminal, condensedNumber * sizeof(CondensedInfo), "fixCondensedTable");

	for (i = 0; i < 256; i++) {
		if (terminalToCondensed[i] != -1) {
			/* Clear the flags, essentially making this a literal */
			condensedToTerminal[terminalToCondensed[i]].flags = 0;
			condensedToTerminal[terminalToCondensed[i]].uLiteral = i;
		}
	}
}

/** Call a function on all @a Directive's.
	@param action The function to execute.
*/
void walkDirectives(void (*action)(Directive *)) {
	int i;
	Declaration *declaration;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == DIRECTIVE)
			/*@-noeffect@*/
			action(declaration->uDirective);
			/*@+noeffect@*/
	}
}

/** Allocate a set number for the set required for a %first macro.
	@param directive The %first directive to allocate a set number for.
*/
void enumerateMacroSets(Directive *directive) {
	Declaration *declaration;
	NonTerminal *nonTerminal;

	if (directive->subtype != FIRST_DIRECTIVE)
		return;

	/* Find the declaration of the non-terminal the %first directive
	   references. */
	declaration = (Declaration *) lookup(globalScope, directive->token[1]->text);
	ASSERT(declaration != NULL && declaration->subtype == NONTERMINAL);
	nonTerminal = declaration->uNonTerminal;

	/* Only allocate a set number for non-empty sets. */
	if (setFill(nonTerminal->term.first) > 1) {
		hasFirst = true;
		setFindIndex(nonTerminal->term.first, true);
	}
}

/** Check a character literal for validity.
	@param literal The @a Token representing the literal.
	@return The character number represented by the literal.

	This function check for empty literals and for invalid escape sequences.
*/
int checkLiteral(Token *literal) {
	size_t length;
	length = strlen(literal->text) - 2;
	if (length == 0) {
		error(literal, "Empty character literal\n");
		return -1;
	}
	switch (literal->text[1]) {
		case '\\':
			if (length == 2) {
				switch (literal->text[2]) {
					/* The original escapes from LLgen (except octal constants) */
					case 'n':
						return '\n';
					case 'r':
						return '\r';
					case '\'':
						return '\'';
					case '\\':
						return '\\';
					case 't':
						return '\t';
					case 'b':
						return '\b';
					case 'f':
						return '\f';
					case '0':
						if (!option.noEOFZero) {
							error(literal, "Zero valued literal\n");
							return -1;
						}
						return 0;
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						return (int) literal->text[2] - '0';
					default:
						break;
				}
				if (option.onlyLLgenEscapes) {
					switch (literal->text[2]) {
						case 'a':
						case 'v':
						case '?':
						case '"':
							if (option.onlyLLgenEscapes)
								error(literal, "Escape sequence %s is incompatible with original LLgen\n", literal->text);
							break;
						default:
							error(literal, "Invalid escape sequence %s\n", literal->text);
					}
					return -1;
				}
				switch (literal->text[2]) {
					case 'a':
						return '\a';
					case 'v':
						return '\v';
					case '?':
						return '?';
					case '"':
						return '"';
					default:
						error(literal, "Invalid escape sequence %s\n", literal->text);
						return -1;
				}
			} else if (length == 3 || length == 4) {
				/* Note: the values for hex escapes can never be too large
				   because the length of the literal is 4. */
				int value;
				char *end;
				if (literal->text[2] == 'x') {
					if (option.onlyLLgenEscapes) {
						error(literal, "Hex escape sequences are incompatible with original LLgen\n");
						return -1;
					}
					value = strtol(literal->text + 3, &end, 16);
					if (*end != '\'') {
						error(literal, "Invalid escape sequence %s\n", literal->text);
						return -1;
					}
				} else if (literal->text[2] >= '0' && literal->text[2] <= '9') {
					value = strtol(literal->text + 3, &end, 8);
					if (*end != '\'' || value > 255) {
						error(literal, "Invalid escape sequence %s\n", literal->text);
						return -1;
					}
				} else {
					error(literal, "Invalid escape sequence %s\n", literal->text);
					return -1;
				}
				if (value == 0 && !option.noEOFZero) {
					error(literal, "Zero valued literal\n");
					return -1;
				}
				return value;
			} else if (literal->text[2] == 'x' && option.onlyLLgenEscapes) {
				error(literal, "Hex escape sequences are incompatible with original LLgen\n");
				return -1;
			}
			error(literal, "Invalid escape sequence %s\n", literal->text);
			return -1;
		default:
			if (length != 1) {
				error(literal, "Invalid character literal %s\n", literal->text + 1);
				return -1;
			}
			return (int) literal->text[1];
	}
}
