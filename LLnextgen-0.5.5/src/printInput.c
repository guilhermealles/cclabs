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

#include <string.h>
#include <errno.h>
#include "globals.h"
#include "nonRuleAnalysis.h"
#include "ruleAnalysis.h"
#include "printInput.h"
#include "option.h"
#define LL_NOTOKENS
#include "grammar.h"

FILE *printRuleOutput;

#if 0
/** Print a @a Directive */
static void printDirective(Directive *directive) {
	switch (directive->subtype) {
		case START_DIRECTIVE:
			fprintf(printRuleOutput, "%%start %s, %s;\n", directive->token[0]->text, directive->token[1]->text);
			break;
		case TOKEN_DIRECTIVE:
			fprintf(printRuleOutput, "%%token %s;\n", directive->token[0]->text);
			break;
		case FIRST_DIRECTIVE:
			fprintf(printRuleOutput, "%%first %s;\n", directive->token[0]->text);
			break;
		case LEXICAL_DIRECTIVE:
			fprintf(printRuleOutput, "%%lexical %s;\n", directive->token[0]->text);
			break;
		case PREFIX_DIRECTIVE:
			fprintf(printRuleOutput, "%%prefix %s;\n", directive->token[0]->text);
			break;
		case ONERROR_DIRECTIVE:
			fprintf(printRuleOutput, "%%onerror %s;\n", directive->token[0]->text);
			break;
		default:
			PANIC();
	}
}
#endif

static int indent;

/** Print indentation, determined by the static variable indent */
static void printIndent(void) {
	int tabs;
	for (tabs = indent; tabs > 0; tabs--)
		fputc('\t', printRuleOutput);
}

/** Print a tokens in a set
	@param name  The name of the token set (First, Follow, Contains)
	@param items The set to be printed

	The routine takes a line length of 80 characthers into account.
*/
static void printSet(const char *name, set items) {
	int lineFill, i;
	bool first = true;
	
	printIndent(); lineFill = 8 * indent;
	indent++;
	fprintf(printRuleOutput, ">> %s {", name); lineFill += strlen(name) + 5;
	/* Loop over all used tokens, and print the ones in the set */
	for (i = 0; i < condensedNumber; i++) {
		if (setContains(items, i)) {
			size_t textLength;
			const char *text;
			if (!first)
				fputc(' ', printRuleOutput);
			else
				first = false;
			if (condensedToTerminal[i].flags & CONDENSED_ISTOKEN) {
				text = condensedToTerminal[i].uToken->token[0]->text;
				textLength = strlen(text) + 1;
			} else if (i == 0) {
				text = "EOFILE";
				textLength = 7;
			} else {
				/* Note: this is a Q&D hack, to access the default symbol table. */
				text = LLgetSymbol(condensedToTerminal[i].uLiteral);
				textLength = strlen(text) + 1;
			}
			if (lineFill + textLength + 1 > 79) {
				fputc('\n', printRuleOutput);
				printIndent(); lineFill = 8 * indent;
			}
			fputs(text, printRuleOutput);
			lineFill += textLength;
		}
	}
	fputs("}\n", printRuleOutput);
	indent--;
}

/** Print a repetition operator
	@param repeats Description of the repetition operator
*/
static void printRepeats(Repeats repeats) {
	switch (repeats.subtype) {
		case FIXED:
			break;
		case STAR:
			if (repeats.number == 1)
				fputc('?', printRuleOutput);
			else
				fputc('*', printRuleOutput);
			break;
		case PLUS:
			fputc('+', printRuleOutput);
			break;
		case FINOPT:
			fputs("..?", printRuleOutput);
			break;
		default:
			PANIC();
	}
	if (repeats.number > 1)
		fprintf(printRuleOutput, " %d", repeats.number);
}

static void printTerm(Term *term);

/** Print the parts of an @a Alternative
	@param alternative The @a Alternative to print

	Note: actions are printed only as {...}
*/
static void printAlternative(Alternative *alternative) {
	GrammarPart *grammarPart;
	int i;
	
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		printIndent();
		switch (grammarPart->subtype) {
			case PART_ACTION:
				fputs("{...}", printRuleOutput);
				break;
			case PART_TERMINAL:
			case PART_LITERAL:
				fputs(grammarPart->token->text, printRuleOutput);
				break;
			case PART_NONTERMINAL:
			case PART_UNDETERMINED:
				fputs(grammarPart->token->text, printRuleOutput);
				break;
			case PART_TERM:
				fputs("[\n", printRuleOutput);
				/* Only print the sets for repeating terms */
				if (!(grammarPart->uTerm.repeats.subtype == FIXED && grammarPart->uTerm.repeats.number == 1)) {
					printSet("First-set", grammarPart->uTerm.first);
					printSet("Contains-set", grammarPart->uTerm.contains);
					printSet("Follow-set", grammarPart->uTerm.follow);
				}
				indent++;
				if (grammarPart->uTerm.flags & TERM_WHILE) {
					printIndent();
					fprintf(printRuleOutput, "%%while %s\n", grammarPart->uTerm.expression->text);
				}
				if (grammarPart->uTerm.flags & TERM_PERSISTENT) {
					printIndent();
					fputs("%persistent\n", printRuleOutput);
				}
				printTerm(&grammarPart->uTerm);
				indent--;
				printIndent();
				fputc(']', printRuleOutput);
				printRepeats(grammarPart->uTerm.repeats);
				break;
			default:
				PANIC();
		}
		fputc('\n', printRuleOutput);
	}
}

/** Print the alternatives of a @a Term
	@param term The @a Term to print
*/
static void printTerm(Term *term) {
	int i;
	Alternative *alternative;
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		if (listSize(term->rule) != 1)
			printSet("Alternative on", alternative->first);
		if (alternative->flags & ALTERNATIVE_PREFER) {
			printIndent();
			fputs("%prefer\n", printRuleOutput);
		}
		if (alternative->flags & ALTERNATIVE_AVOID) {
			printIndent();
			fputs("%avoid\n", printRuleOutput);
		}
		if (alternative->flags & ALTERNATIVE_IF) {
			printIndent();
			fprintf(printRuleOutput, "%%if %s\n", alternative->expression->text);
		}
		if (alternative->flags & ALTERNATIVE_DEFAULT && listSize(term->rule) != 1) {
			printIndent();
			fputs("%default\n", printRuleOutput);
		}
	
		printAlternative(alternative);
		if (i < listSize(term->rule) - 1) {
			indent--;
			printIndent();
			fputs("|\n", printRuleOutput);
			indent++;
		}
	}
}

static bool termContainsConflict(Term *term);
/** Determine whether an Alternative contains a conflict
	@a alternative The Alternative to check.
*/
static bool alternativeContainsConflict(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;
	
	if (alternative->flags & ALTERNATIVE_ACONFLICT)
		return true;
	
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		if (grammarPart->subtype == PART_TERM &&
				termContainsConflict(&grammarPart->uTerm))
			return true;
	}
	return false;
}

/** Determine whether a Term contains a conflict
	@a term The Term to check.
*/
static bool termContainsConflict(Term *term) {
	int i;
	Alternative *alternative;
	
	if (term->flags & TERM_RCONFLICT)
		return true;
	
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		if (alternativeContainsConflict(alternative))
			return true;
	}
	return false;
}

/** Print the contents of a @a NonTerminal
	@param nonTerminal The @a NonTerminal to print
*/
static void printNonTerminal(NonTerminal *nonTerminal) {
	/* For verbosity level 2, only print the rules containing conflicts. */
	if (option.verbose == 2 && !termContainsConflict(&nonTerminal->term))
		return;
	fprintf(printRuleOutput, "%s ", nonTerminal->token->text);
	fputs(":\n", printRuleOutput);
	printSet("First-set", nonTerminal->term.first);
	printSet("Contains-set", nonTerminal->term.contains);
	printSet("Follow-set", nonTerminal->term.follow);
	indent = 1;
	printTerm(&nonTerminal->term);
	fputs(";\n\n", printRuleOutput);
}

/** Print all NonTerminal declarations

	Note: the name is historical, as Directive declarations used to be printed
	as well.
*/
void printDeclarations(void) {
	printRuleOutput = fopen("LL.output", "w");
	if (printRuleOutput == NULL)
		fprintf(stderr, "Could not generate LL.output: %s\n", strerror(errno));
	walkRules(printNonTerminal);
}
