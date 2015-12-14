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

#include "ruleAnalysis.h"
#include "set.h"
#include "globals.h"
#include "traceTokens.h"

/* FIXME: we should probably print the file names as well now, given
	that they don't include the directory names anymore. The best way
	is most likely to print the file names only for non-terminals that
	are not in the file where the conflict is detected, at the site where
	the non-terminal's name is printed. For example:
	
	rule2 @ file.g:10 [ line 73 ] ->
	
*/

static int indentLevel = 1;
/** Print the indentation for the current indentation level. */
static void printIndent(void) {
	int i;
	for (i = 0; i < indentLevel; i++)
		fputs("  ", stderr);
}

/** Reset the NONTERMINAL_TRACED flags for a @a NonTerminal.

	This should be used as the argument to walkRules, to clean up after a
	token trace. All token traces (so both for alternation and repetition
	conflicts) can cause NONTERMINAL_TRACED flags to be set.
*/
static void resetTracedFlags(NonTerminal *nonTerminal) {
	nonTerminal->flags &= ~NONTERMINAL_TRACED;
}

/** Check whether the remaining items of an @a Alternative contain
		conflicting tokens.
	@param alternative The alternative in which the call was found.
	@param i The index at which to start looking for conflicting tokens.
	@param conflicts The set of tokens which may conflict.
	@return A boolean indicating whether a conflicting token was found.
*/
static bool checkFollowSetForTokens(Alternative *alternative, int i, set conflicts) {
	GrammarPart *grammarPart;
	
	/* Walk the list of items in the alternative from the call to the
	   NonTerminal */
	for (; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				break;
			case PART_NONTERMINAL:
				if (!setIntersectionEmpty(conflicts, grammarPart->uNonTerminal.nonTerminal->term.first))
					return true;
				/* If a NonTerminal is nullable, anything following the
				   NonTerminal can also follow the item we started from. */
				if (grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_NULLABLE)
					break;
				return false;
			case PART_TERMINAL:
			case PART_LITERAL:
				/* If the conflicts set doesn't contain this terminal, then
				   the conflict is not here. */
				return setContains(conflicts, grammarPart->uTerminal.terminal);
			case PART_TERM:
				if (!setIntersectionEmpty(conflicts, grammarPart->uTerm.first))
					return true;
				/* If the Term can be skipped, its follow set will contain all
				   the information we need. */
				if ((grammarPart->uTerm.flags & TERM_NULLABLE) || TERM_REPEATS_NULLABLE(grammarPart))
					if (!setIntersectionEmpty(conflicts, grammarPart->uTerm.follow))
						return true;
				return false;
			default:
				PANIC();
		}
	}

	if (!setIntersectionEmpty(conflicts, alternative->enclosing->follow))
		return true;

	/* For repeating Terms, the Term's first set may also follow the item. */
	if ((alternative->enclosing->flags & TERM_OPTMULTREP) &&
			!(alternative->flags & ALTERNATIVE_CONTAINS_FINOPT) &&
			!setIntersectionEmpty(conflicts, alternative->enclosing->first))
		return true;

	if (alternative->enclosing->repeats.subtype == FINOPT) {
		/* Note that this deep digging into the AST is justified because:
		   - an Alternative always has an enclosing Term
		   - a Term that has repeat type FINOPT is always enclosed in another Term
		   - a Term is always part of an Alternative if it is part of a Term rather
			 than a NonTerminal
		*/
		ASSERT(alternative->enclosing->index != THREAD_NONTERMINAL);
		if (!setIntersectionEmpty(conflicts, alternative->enclosing->enclosing.alternative->enclosing->first))
			return true;
	}

	return false;
}

static void traceCallsFromAlternative(Alternative *alternative, NonTerminal *target, set conflicts);
static void traceTermTokens(Term *term, set conflicts);

typedef enum {
	TRACE_IN_FOLLOW,
	TRACE_ALLOW_FOLLOW,
	TRACE_NO_FOLLOW
} TraceMode;

static void traceAlternativeTokensFromIndex(Alternative *alternative, set conflicts, int i, TraceMode mode);

/** Find all the places where a @a NonTerminal is called in a @a Term.
	@param term The @a Term to search.
	@param target The @a NonTerminal to search for.
	@param conflicts The tokens for which a FIRST/FOLLOW conflict in the
		NonTerminal exists.
*/
static void traceCallsFromTerm(Term *term, NonTerminal *target, set conflicts) {
	Alternative *alternative;
	int i;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		traceCallsFromAlternative(alternative, target, conflicts);
	}
}

/** Find all the places where a @a NonTerminal is called in an @a Alternative.
	@param alternative The @a Alternative to search.
	@param target The @a NonTerminal to search for.
	@param conflicts The tokens for which a FIRST/FOLLOW conflict in the
		NonTerminal exists.
*/
static void traceCallsFromAlternative(Alternative *alternative, NonTerminal *target, set conflicts) {
	GrammarPart *grammarPart;
	int i;
	
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_TERM:
				traceCallsFromTerm(&grammarPart->uTerm, target, conflicts);
				break;
			case PART_NONTERMINAL:
				if (grammarPart->uNonTerminal.nonTerminal == target && checkFollowSetForTokens(alternative, i + 1, conflicts)) {
					printIndent();
					fprintf(stderr, "<- %s from %s [ line %d ]\n", target->token->text, getNonTerminalName(alternative->enclosing), grammarPart->token->lineNumber);
					indentLevel++;
					traceAlternativeTokensFromIndex(alternative, conflicts, i + 1, TRACE_IN_FOLLOW);
					indentLevel--;
				}
				break;
			default:
				break;
		}
	}
}

/** Find all the places where a @a NonTerminal is called.
	@param nonTerminal The @a NonTerminal to search for.
	@param conflicts The tokens for which a FIRST/FOLLOW conflict in the
		NonTerminal exists.
*/
static void traceNonTerminalFollowSetTokens(NonTerminal *nonTerminal, set conflicts) {
	Declaration *declaration;
	int i;
	
	if (nonTerminal->flags & NONTERMINAL_TRACING) {
		printIndent();
		fprintf(stderr, "<- %s (recursive)\n", nonTerminal->token->text);
		return;
	} else if (nonTerminal->flags & NONTERMINAL_TRACED) {
		printIndent();
		fprintf(stderr, "<- %s (already printed)\n", nonTerminal->token->text);
		return;
	}
	/* Make sure we don't end up in an infinite loop. */
	nonTerminal->flags |= NONTERMINAL_TRACING | NONTERMINAL_TRACED;

	/* Iterate over all NonTerminals */
	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == NONTERMINAL)
			traceCallsFromTerm(&declaration->uNonTerminal->term, nonTerminal, conflicts);
	}
	nonTerminal->flags &= ~NONTERMINAL_TRACING;
}

/** Find the tokens in the follow set of an @a Alternative that conflict with
		a first set.
	@param alternative The @a Alternative for which to search the follow set.
	@param conflicts The tokens for which a FIRST/FOLLOW conflict.
	@param forceFinopt Force checking of repeating @a Term's first set,
		eventhough the alternative contains a FINOPT operator.
*/
static void traceAlternativeFollowSetTokens(Alternative *alternative, set conflicts, bool forceFinopt) {
	/* Note that when this routine gets called, the alternative's tail is nullable. */
	int index;
	
	index = alternative->enclosing->index;
	if (index == THREAD_NONTERMINAL) {
		if ((alternative->enclosing->enclosing.nonTerminal->flags & NONTERMINAL_START) &&
				setContains(conflicts, terminalToCondensed[256])) {
			printIndent();
			fprintf(stderr, "EOFILE (from %%start directive)\n");
		}
		traceNonTerminalFollowSetTokens(alternative->enclosing->enclosing.nonTerminal, conflicts);
		return;
	}
	
	if (alternative->enclosing->flags & TERM_OPTMULTREP &&
			(forceFinopt || !(alternative->flags & ALTERNATIVE_CONTAINS_FINOPT)) &&
			!setIntersectionEmpty(conflicts, alternative->enclosing->first)) {
		GrammarPart *grammarPart = (GrammarPart *) listIndex(alternative->enclosing->enclosing.alternative->parts, alternative->enclosing->index);
		printIndent();
		fprintf(stderr, "Enclosing term [ line %d ] may repeat, therefore\n", grammarPart->token->lineNumber);
		printIndent();
		fprintf(stderr, "        term's first set is part of follow set:\n");
		indentLevel++;
		traceTermTokens(alternative->enclosing, conflicts);
		indentLevel--;
	}

	if (alternative->enclosing->repeats.subtype == FINOPT) {
		/* Skip the call to traceAlternativeTokensFromIndex, because we know the
		   FINOPT item is the last anyway, and we need to force the check for the
		   enclosing term's first set. Otherwise the check will stop because the
		   follow set of the FINOPT term does not include the enclosing term's
		   first set, and for items before the FINOPT term the first set from the
		   enclosing term should not be checked. (In the case where there can be
		   a conflict from the items before the FINOPT term, it is a conflict with
		   something following the enclosing term and NOT the enclosing term's
		   first set because this would mean the FINOPT is skipped, which should
		   result in the repetition stopping.) */

		/* Note that this deep digging into the AST is justified because:
		   - an Alternative always has an enclosing Term
		   - a Term that has repeat type FINOPT is always enclosed in another Term
		   - a Term is always part of an Alternative if it is part of a Term rather
			 than a NonTerminal
		*/
		ASSERT(alternative->enclosing->index != THREAD_NONTERMINAL);
		traceAlternativeFollowSetTokens(alternative->enclosing->enclosing.alternative, conflicts, true);
		return;
	}

	/* No need to do more, if the follow set of the enclosing Term doesn't
	   contain any of the conflicting tokens. */
	if (setIntersectionEmpty(conflicts, alternative->enclosing->follow))
		return;
	alternative = alternative->enclosing->enclosing.alternative;
	traceAlternativeTokensFromIndex(alternative, conflicts, index + 1, TRACE_IN_FOLLOW);
}

/** Find the conflicting tokens in an @a Alternative, from a specified item.
	@param alternative The @a Alternative to scan.
	@param conflicts The set of conflicting tokens to search for.
	@param i The index to start the search at.
	@param mode The way to handle tokens from the follow set.

	The @a mode parameter can have the following values:
	@li @a TRACE_IN_FOLLOW used when tracing the follow set.
	@li @a TRACE_ALLOW_FOLLOW used when tracing the first set.
	@li @a TRACE_NO_FOLLOW used when already tracing the follow set, or when
		tracing deeper into the AST. This is used to prevent double prints of
		sets/infinite recursion.
*/
static void traceAlternativeTokensFromIndex(Alternative *alternative, set conflicts, int i, TraceMode mode) {
	GrammarPart *grammarPart;
	
	for (; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				break;
			case PART_NONTERMINAL:
				if (!setIntersectionEmpty(conflicts, grammarPart->uNonTerminal.nonTerminal->term.first)) {
					printIndent();
					if (!(grammarPart->uNonTerminal.nonTerminal->flags & NONTERMINAL_VISITED)) {
						fprintf(stderr, "%s [ line %d ] ->\n", grammarPart->token->text, grammarPart->token->lineNumber);
						
						/* Set the visited flag to prevent infinite recursion,
						   and trace the non-terminal */
						grammarPart->uNonTerminal.nonTerminal->flags |= NONTERMINAL_VISITED;
						indentLevel++;
						traceTermTokens(&grammarPart->uNonTerminal.nonTerminal->term, conflicts);
						indentLevel--;
						grammarPart->uNonTerminal.nonTerminal->flags &= ~NONTERMINAL_VISITED;
					} else {
						fprintf(stderr, "%s [ line %d ] (already printed)\n", grammarPart->token->text, grammarPart->token->lineNumber);
					}
				}
				if (grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_NULLABLE)
					break;
				return;
			case PART_TERMINAL:
			case PART_LITERAL:
				if (setContains(conflicts, grammarPart->uTerminal.terminal)) {
					printIndent();
					fprintf(stderr, "%s [ line %d ]\n", grammarPart->token->text, grammarPart->token->lineNumber);
				}
				return;
			case PART_TERM:
				traceTermTokens(&grammarPart->uTerm, conflicts);
				if (grammarPart->uTerm.flags & TERM_NULLABLE || TERM_REPEATS_NULLABLE(grammarPart))
					break;
				return;
			default:
				PANIC();
		}
	}

	if (mode == TRACE_NO_FOLLOW || !checkFollowSetForTokens(alternative, i, conflicts))
		return;

	/* Check the follow set, if the mode allows it. */
	if (mode == TRACE_ALLOW_FOLLOW)
		fprintf(stderr, "  From the follow set:\n");

	traceAlternativeFollowSetTokens(alternative, conflicts, false);
}

/** Find the conflicting tokens in an @a Alternative.
	@param alternative The @a Alternative to scan.
	@param conflicts The set of conflicting tokens to search for.
*/
void traceAlternativeTokens(Alternative *alternative, set conflicts) {
	traceAlternativeTokensFromIndex(alternative, conflicts, 0, TRACE_ALLOW_FOLLOW);
	walkRules(resetTracedFlags);
}

/** Find the conflicting tokens in a @a Term.
	@param term The @a Term to scan.
	@param conflicts The set of conflicting tokens to search for.
*/
static void traceTermTokens(Term *term, set conflicts) {
	Alternative *alternative;
	int i;
	
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		traceAlternativeTokensFromIndex(alternative, conflicts, 0, TRACE_NO_FOLLOW);
	}
}

/** Show all tokens contributing to a repetition conflict.
	@param term The @a Term containing the repetition conflict.
*/
void traceTermRepetitionConflict(Term *term) {
	Alternative *alternative;
	int i;
	set conflicts;
	
	conflicts = newSet(condensedNumber);
	setCopy(conflicts, term->first);
	setIntersection(conflicts, term->follow);
	
	fprintf(stderr, "Trace for the conflicting tokens from the first set:\n");
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		traceAlternativeTokensFromIndex(alternative, conflicts, 0, TRACE_NO_FOLLOW);
	}
	walkRules(resetTracedFlags);

	fprintf(stderr, "Trace for the conflicting tokens from the follow set:\n");
	if (term->index == THREAD_NONTERMINAL)
		traceNonTerminalFollowSetTokens(term->enclosing.nonTerminal, conflicts);
	else
		traceAlternativeTokensFromIndex(term->enclosing.alternative, conflicts, term->index + 1, TRACE_IN_FOLLOW);
	walkRules(resetTracedFlags);
}
