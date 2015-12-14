/* Copyright (C) 2005-2009 G.P. Halkes
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

#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "ruleAnalysis.h"
#include "globals.h"
#include "list.h"
#include "argcount.h"
#include "option.h"
#include "traceTokens.h"
#include "clash.h"

#include "posixregex.h"

/** Allocate and initialise a new @a NonTerminal.
	@param token The @a Token representing the name of the NonTerminal.
	@param parameters The optional parameters.
	@param localDeclarations The optional local variable declarations.
	@param rule The @a List of @a Alternatives making up this NonTerminal.
	@param argCount The number of arguments in this alternative.
	@return A pointer to an initialised @a NonTerminal struct.

	This function does not return if the required memory can not be allocated.
*/
NonTerminal *newNonTerminal(Token *token, Token *retvalIdent, CCode *parameters, CCode *localDeclarations, List *rule, int argCount) {
	NonTerminal *retval;

	retval = (NonTerminal *) safeCalloc(sizeof(NonTerminal), "newNonTerminal");
	retval->token = token;
	retval->retvalIdent = retvalIdent;
	retval->parameters = parameters;
	retval->declarations = localDeclarations;
	retval->argCount = argCount;
	retval->term.rule = rule;
	retval->term.length = INT_MAX; /* Not know yet. */
	retval->term.index = THREAD_NONTERMINAL;
	retval->term.enclosing.nonTerminal = retval;
	return retval;
}

/** Allocate and initialise a new @a Alternative.
	@return A pointer to an initialised @a Alternative struct.

	This function does not return if the required memory can not be allocated.
*/
Alternative *newAlternative(void) {
	Alternative *retval;

	retval = (Alternative *) safeCalloc(sizeof(Alternative), "newAlternative");
	retval->parts = newList();
	retval->token = newToken();
	retval->length = INT_MAX; /* Not known yet. */
	retval->label = -1; /* Not known yet. */
	return retval;
}

/** Allocate and initialise a new @a GrammarPart.
	@param token The token representing the @a GrammarPart.
	@param subtype The @a GrammarPart subtype.
	@return A pointer to an initialised @a GrammarPart struct.

	The subtype must be one of @a PART_UNKNOWN, @a PART_ACTION,
	@a PART_NONTERMINAL, @a PART_TERMINAL, @a PART_LITERAL,
	@a PART_UNDETERMINED, @a PART_TERM.

	This function does not return if the required memory can not be allocated.
*/
GrammarPart *newGrammarPart(Token *token, GrammarPartSubtype subtype) {
	GrammarPart * retval;

	retval = (GrammarPart *) safeCalloc(sizeof(GrammarPart), "newGrammarPart");

	retval->token = token;
	retval->subtype = subtype;
	if (subtype == PART_TERM) {
		retval->uTerm.rule = newList();
		retval->uTerm.length = INT_MAX;
	}

	return retval;
}

/** Create a @a Term for a @a GrammarPart that is to repeat.
	@param grammarPart The @a GrammarPart to wrap.

	If a user writes FOO+ in the grammar, a @a GrammarPart is created for FOO.
	However, to make processing easier, this @a GrammarPart is then wrapped in
	a @a Term to make sure only @a Terms can repeat.
*/
GrammarPart *wrapInTerm(GrammarPart *grammarPart) {
	GrammarPart *term;
	Alternative *alternative;

	term = newGrammarPart(grammarPart->token, PART_TERM);

	/* The reason I don't call newAlternative here is because it creates a new
	   Token, and that is unnecessary and creates the wrong token */
	alternative = (Alternative *) safeCalloc(sizeof(Alternative), "wrapInTerm");
	alternative->parts = newList();
	alternative->token = grammarPart->token;
	alternative->length = INT_MAX;
	alternative->enclosing = &term->uTerm;

	listAppend(alternative->parts, grammarPart);
	listAppend(term->uTerm.rule, alternative);

#ifdef MEM_DEBUG
	term->dontFreeToken = true;
	alternative->dontFreeToken = true;
#endif

	return term;
}


static GrammarPart *copyGrammarPart(GrammarPart *grammarPart, Token *operator, Alternative *alternative);

/** Copy an @a Alternative and all it's children.
	@param alternative The @a Alternative to copy.
	@param operator The @a Token which required this copy.
	@param term The enclosing @a Term.
*/
static Alternative *copyAlternative(Alternative *alternative, Token *operator, Term *term) {
	Alternative *copy;
	int i;

	copy = (Alternative *) safeCalloc(sizeof(Alternative), "copyAlternative");
	memcpy(copy, alternative, sizeof(Alternative));

	/* Initialise members. */
	copy->parts = newList();
	copy->enclosing = term;
	copy->token = copyToken(alternative->token, operator->fileName, operator->lineNumber);
#ifdef MEM_DEBUG
	copy->dontFreeToken = false;
#endif

	copy->expression = copyCCode(alternative->expression, operator->fileName, operator->lineNumber);

	for (i = 0; i < listSize(alternative->parts); i++)
		listAppend(copy->parts, copyGrammarPart(listIndex(alternative->parts, i), operator, copy));

	return copy;
}

/** Copy a @a GrammarPart and all it's children.
	@param grammarPart The @a GrammarPart to copy.
	@param operator The @a Token which required this copy.
	@param alternative The enclosing @a Alternative.

	Copying a @a GrammarPart is trivial except for the @a Term case. For the
	@a Term case we need to do deep copy. The reason for this is twofold:
	firstly, nullable items may cause different first and follow sets to be
	computed for the copy. Secondly, we want to do accurate reporting of error
	locations.
*/
static GrammarPart *copyGrammarPart(GrammarPart *grammarPart, Token *operator, Alternative *alternative) {
	GrammarPart *copy;
	int i;

	copy = (GrammarPart *) safeMalloc(sizeof(GrammarPart), "copyGrammarPart");
	memcpy(copy, grammarPart, sizeof(GrammarPart));

	/* Initialise members that differ from original. */
	copy->token = copyToken(grammarPart->token, operator->fileName, operator->lineNumber);
#ifdef MEM_DEBUG
	copy->dontFreeToken = false;
#endif

	switch (copy->subtype) {
		case PART_TERM:
			copy->uTerm.rule = newList();
			copy->uTerm.enclosing.alternative = alternative;
			copy->uTerm.expression = copyCCode(grammarPart->uTerm.expression, operator->fileName, operator->lineNumber);

			for (i = 0; i < listSize(grammarPart->uTerm.rule); i++)
				listAppend(copy->uTerm.rule, copyAlternative((Alternative *) listIndex(grammarPart->uTerm.rule, i), operator, &copy->uTerm));
			break;
		case PART_ACTION:
			copy->uAction = copyCCode(grammarPart->uAction, operator->fileName, operator->lineNumber);
			break;
		case PART_NONTERMINAL:
			copy->uNonTerminal.expression = copyCCode(grammarPart->uNonTerminal.expression, operator->fileName, operator->lineNumber);
			copy->uNonTerminal.retvalIdent = copyToken(grammarPart->uNonTerminal.retvalIdent, operator->fileName, operator->lineNumber);
			break;
		case PART_LITERAL:
		case PART_TERMINAL:
		case PART_UNKNOWN:
		case PART_UNDETERMINED: /* Only happens when lookup failed. */
			break;
		default:
			PANIC();
	}

	return copy;
}

/** Retrieve the non-terminal a @a Term is part of.
	@param term The @a Term to find the @a NonTerminal for.
	@return The @a NonTerminal.
*/
NonTerminal *getEnclosingNonTerminal(Term *term) {
	while (term->index != THREAD_NONTERMINAL) term = term->enclosing.alternative->enclosing;
	return term->enclosing.nonTerminal;
}

static bool onlyReachable = false;

/** Set the @a onlyReachable flag to a specified value.
	@param newValue The new value for the @a onlyReachable flag.

	This function exists so we can more easily implement an option to turn
	the selective checking off.
*/
void setOnlyReachable(bool newValue) {
	if (!option.notOnlyReachable)
		onlyReachable = newValue;
}

static void resetReachableForNonTerminals(void);

/** Execute a transitive closure on all @a NonTerminals.
	@param nonTerminalAction The function to call on all @a NonTerminals.

	The @a nonTerminalAction function should return @a true if something
	relavant to the algorithm being executed has changed. As long as an
	iteration over all @a NonTerminals has one of the calls to
	@a nonTerminalAction return true, a next iteration is started.
*/
static void transitiveClosure(bool (*nonTerminalAction)(NonTerminal *)) {
	bool changed;
	int i;
	Declaration *declaration;

	do {
		changed = false;
		for (i = 0; i < listSize(declarations); i++) {
			declaration = (Declaration *) listIndex(declarations, i);
			if (declaration->subtype == NONTERMINAL && (!onlyReachable || (declaration->uNonTerminal->flags & NONTERMINAL_REACHABLE)))
				changed |= nonTerminalAction(declaration->uNonTerminal);
		}
	} while (changed);
}

/** Execute a transitive closure on all @a NonTerminals.
	@param termAction The function to call on all @a NonTerminals.

	The @a termAction function should return @a true if something
	relavant to the algorithm being executed has changed. As long as an
	iteration over all @a NonTerminals has one of the calls to
	@a nonTerminalAction return true, a next iteration is started.

	The difference with @a transitiveClosure is that this function uses the
	@a Term that is in the @a NonTerminal as the object of the action, instead
	of the @a NonTerminal itself. This obviates the need for many functions
	which have this as their sole purpose.
*/
static void transitiveClosureSimple(bool (*termAction)(Term *)) {
	bool changed;
	int i;
	Declaration *declaration;

	do {
		changed = false;
		for (i = 0; i < listSize(declarations); i++) {
			declaration = (Declaration *) listIndex(declarations, i);
			if (declaration->subtype == NONTERMINAL && (!onlyReachable || (declaration->uNonTerminal->flags & NONTERMINAL_REACHABLE)))
				changed |= termAction(&declaration->uNonTerminal->term);
		}
	} while (changed);
}

/** Do a single iteration over all the @a NonTerminals.
	@param nonTerminalAction The function to execute on the @a NonTerminals.
*/
void walkRules(void (*nonTerminalAction)(NonTerminal *)) {
	int i;
	Declaration *declaration;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == NONTERMINAL && (!onlyReachable || (declaration->uNonTerminal->flags & NONTERMINAL_REACHABLE)))
			/*@-noeffect@*/
			nonTerminalAction(declaration->uNonTerminal);
			/*@+noeffect@*/
	}
}

/** Do a single iteration over all the @a NonTerminals.
	@param termAction The function to execute on the @a NonTerminals.

	This function is a simplification of the @a walkRules function, such that
	in the frequent case where the action only needs to consider the @a Term
	embedded in the @a NonTerminal we don't need an extra function to achieve
	this.
*/
void walkRulesSimple(void (*termAction)(Term *)) {
	int i;
	Declaration *declaration;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == NONTERMINAL && (!onlyReachable || (declaration->uNonTerminal->flags & NONTERMINAL_REACHABLE)))
			/*@-noeffect@*/
			termAction(&declaration->uNonTerminal->term);
			/*@+noeffect@*/
	}
}

static bool termNullability(Term *term);
static bool alternativeNullability(Alternative *alternative);

/** Find all @a Terms and @a Alternatives that MAY match nothing. */
void determineNullability(void) {
	transitiveClosureSimple(termNullability);
}

/** Determine nullability of a @a Term. */
static bool termNullability(Term *term) {
	/* Note: this indicates that an alternative has	changed */
	bool changed = false;
	int i;
	Alternative *alternative;

	/* Determine nullability for all Alternatives in the Term. */
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		changed |= alternativeNullability(alternative);
	}
	/* If nothing has changed, or the nullable flag has already been set,
	   we're done. Otherwise, one of the Alternatives has had its nullable
	   flag set, and therefore this Term is nullable. */
	if (!changed || (term->flags & TERM_NULLABLE))
		return false;

	term->flags |= TERM_NULLABLE;
	return true;
}

/** Determine nullability of an @a Alternative. */
static bool alternativeNullability(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		if (grammarPart->subtype == PART_TERM)
			termNullability(&grammarPart->uTerm);
	}
	/* If the nullable flag has already been set, we're done */
	if (alternative->flags & ALTERNATIVE_NULLABLE)
		return false;
	/* Check whether all parts of the alternative are nullable */
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		/* A non-nullable part means the alternative is non-nullable */
		if (!((grammarPart->subtype == PART_NONTERMINAL && (grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_NULLABLE)) ||
				(grammarPart->subtype == PART_TERM && ((grammarPart->uTerm.flags & TERM_NULLABLE) || TERM_REPEATS_NULLABLE(grammarPart))) ||
				(grammarPart->subtype == PART_ACTION))) {
			return false;
		}
	}
	/* All parts are nullable, if we get here */
	alternative->flags |= ALTERNATIVE_NULLABLE;
	return true;
}

static bool termComputeFirstSet(Term *term);
static bool alternativeComputeFirstSet(Alternative *alternative);

/** Compute the first sets for all @a Terms and @a Alternatives.

	The first set is the set of tokens a @a Term or @a Alternative can start
	with.
*/
void computeFirstSets(void) {
	transitiveClosureSimple(termComputeFirstSet);
}

/** Compute the first set of a @a Term. */
static bool termComputeFirstSet(Term *term) {
	bool changed = false;
	int i;
	Alternative *alternative;

	/* The first set of a Term is the union of all the first sets of its
	   constituent Alternatives. */
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		if (alternativeComputeFirstSet(alternative))
			changed |= setUnion(term->first, alternative->first);
	}
	return changed;
}

/** Determine the first sets for ALL the parts in the alternative. */
static bool alternativeComputeFirstSet(Alternative *alternative) {
	bool changed = false;
	int i;
	GrammarPart *grammarPart;

	/* First determine all the first sets for all parts */
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		if (grammarPart->subtype == PART_TERM)
			termComputeFirstSet(&grammarPart->uTerm);
	}
	/* Note: We cannot stop here, because then we won't set anything when
	   this is the first time this is called on the alternative */

	/* Then compute the first set of this alternative */
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				break;
			case PART_NONTERMINAL:
				changed |= setUnion(alternative->first, grammarPart->uNonTerminal.nonTerminal->term.first);
				if (!(grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_NULLABLE))
					return changed;
				break;
			case PART_TERMINAL:
			case PART_LITERAL:
				changed |= setSet(alternative->first, grammarPart->uTerminal.terminal);
				return changed;
			case PART_TERM:
				changed |= setUnion(alternative->first, grammarPart->uTerm.first);
				/* Note: TERM_REPEATS_NULLABLE checks are necessary because this
				   information is not contained in the TERM_NULLABLE flag. */
				if (!((grammarPart->uTerm.flags & TERM_NULLABLE) || TERM_REPEATS_NULLABLE(grammarPart)))
					return changed;
				break;
			default:
				PANIC();
		}
	}
	return changed;
}

static bool nonTerminalComputeFollowSet(NonTerminal *nonTerminal);
static bool termComputeFollowSet(Term *term, const set follow);
static bool alternativeComputeFollowSet(Alternative *alternative, const set follow);

/** Compute the first sets for all @a Terms and @a Alternatives.

	The follow set is the set of tokens that can follow a @a Term or
	@a Alternative. These are of importance for repeating terms, recursion and
	error-handling.
*/
void computeFollowSets(void) {
	transitiveClosure(nonTerminalComputeFollowSet);
}

/** Compute the follow set for a @a NonTerminal. */
static bool nonTerminalComputeFollowSet(NonTerminal *nonTerminal) {
	return termComputeFollowSet(&nonTerminal->term, nonTerminal->term.follow);
}

/** Compute the follow set for a @a Term.
	@param term The @a Term to compute the follow set for.
	@param follow The accumulated set of tokens that can follow this @a Term.

	The @a follow set is not the same as the @a Term's follow set, because the
	@a Term's follow set is strictly what can follow this term, while the
	@a follow set can also contain the @a Term's first set if the @a Term
	can repeat.
*/
static bool termComputeFollowSet(Term *term, const set follow) {
	Alternative *alternative;
	bool changed = false;
	int i;

	/* Note: the follow set of a term is actually determined in the follow set
	   calculation of the alternatives, both under PART_TERM and under
	   PART_NONTERMINAL! */
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		changed |= alternativeComputeFollowSet(alternative, follow);
	}
	return changed;
}

/** Compute the follow sets for an @a Alternative's parts.
	@param alternative The @a Alternative to compute the follow sets for.
	@param follow The accumulated set of tokens that can follow this
		@a Alternative.

	Note: the @a Alternative itself doesn't have a follow set, as this is
	determined by the enclosing @a Term.
*/
static bool alternativeComputeFollowSet(Alternative *alternative, const set follow) {
	GrammarPart *grammarPart;
	set currentFollow;
	bool changed = false;
	int i;

	currentFollow = newSet(condensedNumber);
	setCopy(currentFollow, follow);

	for (i = listSize(alternative->parts) - 1; i >= 0; i--) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				break;
			case PART_NONTERMINAL:
				/* Add the current follow set to the follow set of the non-terminal */
				changed |= setUnion(grammarPart->uNonTerminal.nonTerminal->term.follow, currentFollow);
				/* If the non-terminal may be skipped, use both the current follow set and
				   the non-terminal's first set as the new follow set. Otherwise just use
				   the non-terminal's fist set. */
				if (grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_NULLABLE)
					setUnion(currentFollow, grammarPart->uNonTerminal.nonTerminal->term.first);
				else
					setCopy(currentFollow, grammarPart->uNonTerminal.nonTerminal->term.first);
				break;
			case PART_TERMINAL:
			case PART_LITERAL:
				/* A terminal may not be skipped, so use only the terminal as the current
				   follow set. */
				setClear(currentFollow);
				setSet(currentFollow, grammarPart->uTerminal.terminal);
				break;
			case PART_TERM:
				/* For fin-opt term's its all a little different. It's follow set is
				   only the follow set of the enclosing term. However, for the contents
				   the follow set is the regular follow set.
				   For anything before the fin-opt term, the follow set consists of only
				   the follow set of the enclosing term and the first set of the fin-opt
				   term.
				   The follow set of the enclosing term does not include the enclosing
				   terms first set, which the currentFollow does include because the term
				   does repeat. */
				if (grammarPart->uTerm.repeats.subtype == FINOPT) {
					changed |= setUnion(grammarPart->uTerm.follow, alternative->enclosing->follow);
					changed |= termComputeFollowSet(&grammarPart->uTerm, currentFollow);
					setCopy(currentFollow, alternative->enclosing->follow);
					setUnion(currentFollow, grammarPart->uTerm.first);
					break;
				}

				/* Add the current follow set to the follow set of the term */
				changed |= setUnion(grammarPart->uTerm.follow, currentFollow);
				/* If this term appears in the grammar with a (possible)
				   repeat count larger than zero, the term may in effect also
				   follow itself. Therefore, we need to add the term's first set
				   to the follow set in those cases. */
				if (grammarPart->uTerm.flags & TERM_OPTMULTREP)
					setUnion(currentFollow, grammarPart->uTerm.first);
				changed |= termComputeFollowSet(&grammarPart->uTerm, currentFollow);
				/* If the term may be skipped, use both the current follow set and
				   the term's first set as the new follow set. Otherwise just use
				   the term's fist set. */
				if ((grammarPart->uTerm.flags & TERM_NULLABLE) || TERM_REPEATS_NULLABLE(grammarPart))
					setUnion(currentFollow, grammarPart->uTerm.first);
				else
					setCopy(currentFollow, grammarPart->uTerm.first);
				break;
			default:
				PANIC();
		}
	}
	deleteSet(currentFollow);
	return changed;
}

/** Add two lengths together, taking infinity into account.
	@param a The first length, and the place to store the result.
	@param b The second length.
	@param factor The factor to multiply @a b with.

	This routine caculates @a a + @a b * @a factor. However, as both @a and
	@a b can be infinity (denoted by INT_MAX) some care has to be taken to
	ensure that calculations don't wrap round.
*/
static void addLength(int *a, int b, int factor) {
	if ((INT_MAX/factor) < b) {
		*a = INT_MAX;
		return;
	}

	b *= factor;
	if (INT_MAX - b <= *a) {
		*a = INT_MAX;
		return;
	}

	*a = b + *a;
}

static bool termComputeLength(Term *term);
static bool alternativeComputeLength(Alternative *alternative);

/** Compute the lengths of all @a NonTerminals and @a Terms.

	The lengths are computed to determine the default choices. The length is
	computed as the minimum length required to fully match the @a NonTerminal
	or @a Term. Contrary to LLgen, LLnextgen doesn't take the rule complexity
	into account.
*/
void computeLengths(void) {
	int i;
	Declaration *declaration;

	transitiveClosureSimple(termComputeLength);
	/* Find and mark the non-terminals which have recursive default choices.
	   These can easily be found because the minimum length equals "infinity",
	   signalled by INT_MAX. */
	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == NONTERMINAL && declaration->uNonTerminal->term.length == INT_MAX) {
			declaration->uNonTerminal->flags |= NONTERMINAL_RECURSIVE_DEFAULT;
		}
	}
}

/** Compute the length of a @a Term. */
static bool termComputeLength(Term *term) {
	bool changed = false;
	int i, min = INT_MAX;
	Alternative *alternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		changed |= alternativeComputeLength(alternative);
	}
	/* What we need to do, is set the size of this term to the size of the
	   default choice if one is available. Otherwise, we need to set it to
	   the shortest alternative we can find. LLgen also takes a complexity
	   metric into account, but this makes the choice opaque. */
	if (changed) {
		for (i = 0; i < listSize(term->rule); i++) {
			alternative = (Alternative *) listIndex(term->rule, i);
			if (alternative->flags & ALTERNATIVE_DEFAULT) {
				min = alternative->length;
				break;
			} else if (alternative->length < min) {
				min = alternative->length;
			}
		}
	}
	if (term->length > min) {
		changed = true;
		term->length = min;
	}
	return changed;
}

/** Compute the length of an @a Alternative. */
static bool alternativeComputeLength(Alternative *alternative) {
	bool changed = false;
	int i, size = 0;
	GrammarPart *grammarPart;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				break;
			case PART_LITERAL:
			case PART_TERMINAL:
				addLength(&size, 1, 1);
				break;
			case PART_NONTERMINAL:
				addLength(&size, grammarPart->uNonTerminal.nonTerminal->term.length, 1);
				break;
			case PART_TERM:
				termComputeLength(&grammarPart->uTerm);
				if (TERM_REPEATS_NULLABLE(grammarPart) && !(grammarPart->uTerm.flags & TERM_PERSISTENT))
					break;
				addLength(&size, grammarPart->uTerm.length, grammarPart->uTerm.repeats.subtype == FIXED ? grammarPart->uTerm.repeats.number : 1);
				break;
			default:
				PANIC();
		}
	}
	if (alternative->length != size) {
		changed = true;
		alternative->length = size;
	}
	return changed;
}

/** Initialise the @a NONTERMINAL_REACHABLE flags from the @a NONTERMINAL_START
		and @a NONTERMINAL_FIRST flags.

	This function has to be called through walkRules before determining
	reachability.
*/
static void nonTerminalReachabilityInit(NonTerminal *nonTerminal) {
	if (nonTerminal->flags & (NONTERMINAL_START|NONTERMINAL_FIRST))
		nonTerminal->flags |= NONTERMINAL_REACHABLE;
}

/** Initialise the @a NONTERMINAL_REACHABLE flags from the @a NONTERMINAL_START
		flags.

	This function has to be called through walkRules before determining
	reachability.
*/
static void nonTerminalCodeReachabilityInit(NonTerminal *nonTerminal) {
	if (nonTerminal->flags & NONTERMINAL_START)
		nonTerminal->flags |= NONTERMINAL_REACHABLE;
}

static bool nonTerminalReachability(NonTerminal *nonTerminal);
static bool termReachability(Term *term);
static bool alternativeReachability(Alternative *alternative);

/** Determine which @a NonTerminals and @a Tokens are reachable from this
		@a NonTerminal.

	Note: this is called from findConflicts.
*/
static bool nonTerminalReachability(NonTerminal *nonTerminal) {
	/* Only use this NonTerminal to determine reachability, if it is itself
	   reachable. */
	if (nonTerminal->flags & NONTERMINAL_REACHABLE)
		return termReachability(&nonTerminal->term);
	return false;
}

/** Determine which @a NonTerminals and @a Tokens are reachable from this
		@a Term. */
static bool termReachability(Term *term) {
	bool changed = false;
	int i;
	Alternative *alternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		/* Alternatives that are never chosen do not add to the
		   reachibility of non-terminals and tokens. */
		if (!(alternative->flags & ALTERNATIVE_NEVER_CHOSEN))
			changed |= alternativeReachability(alternative);
	}
	return changed;
}

/** Determine which @a NonTerminals and @a Tokens are reachable from this
		@a Alternative. */
static bool alternativeReachability(Alternative *alternative) {
	bool changed = false;
	int i;
	GrammarPart *grammarPart;

	for (i = listSize(alternative->parts) - 1; i >= 0; i--) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
			case PART_LITERAL:
				break;
			case PART_NONTERMINAL:
				if (!(grammarPart->uNonTerminal.nonTerminal->flags & NONTERMINAL_REACHABLE)) {
					changed = true;
					grammarPart->uNonTerminal.nonTerminal->flags |= NONTERMINAL_REACHABLE;
				}
				break;
			case PART_TERMINAL:
				condensedToTerminal[grammarPart->uTerminal.terminal].flags |= CONDENSED_REACHABLE;
				break;
			case PART_TERM:
				changed |= termReachability(&grammarPart->uTerm);
				break;
			default:
				PANIC();
		}
	}
	return changed;
}

static bool nonTerminalReachabilitySimple(NonTerminal *nonTerminal);
static bool termReachabilitySimple(Term *term);
static bool alternativeReachabilitySimple(Alternative *alternative);

/** Determine which @a NonTerminals are reachable.

	WARNING: This does not calculate the completely correct values for the
	@a NONTERMINAL_REACHABLE flags, but a conservative estimate. In the case
	where there are @a Alternatives that are never chosen, this function will
	still consider any @a NonTerminals named in the @a Alternative reachable.
	This case is rare. The reason for this function is to avoid having to do
	analysis on clearly unreachable @a NonTerminals, and preventing conflicts
	arising from those unreachable @a NonTerminals. Also note that if this
	function determines that a @a NonTerminal is unreachable, it really is.
*/
void determineReachabilitySimple(void) {
	walkRules(nonTerminalReachabilityInit);
	transitiveClosure(nonTerminalReachabilitySimple);
}

/** Determine which @a NonTerminals are reachable from this @a NonTerminal.

	Note: this is called from findConflicts.
*/
static bool nonTerminalReachabilitySimple(NonTerminal *nonTerminal) {
	/* Only use this NonTerminal to determine reachability, if it is itself
	   reachable. */
	if (nonTerminal->flags & NONTERMINAL_REACHABLE)
		return termReachabilitySimple(&nonTerminal->term);
	return false;
}

/** Determine which @a NonTerminals are reachable from this @a Term. */
static bool termReachabilitySimple(Term *term) {
	bool changed = false;
	int i;
	Alternative *alternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		changed |= alternativeReachabilitySimple(alternative);
	}
	return changed;
}

/** Determine which @a NonTerminals are reachable from this @a Alternative. */
static bool alternativeReachabilitySimple(Alternative *alternative) {
	bool changed = false;
	int i;
	GrammarPart *grammarPart;

	for (i = listSize(alternative->parts) - 1; i >= 0; i--) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
			case PART_LITERAL:
			case PART_TERMINAL:
				break;
			case PART_NONTERMINAL:
				if (!(grammarPart->uNonTerminal.nonTerminal->flags & NONTERMINAL_REACHABLE)) {
					changed = true;
					grammarPart->uNonTerminal.nonTerminal->flags |= NONTERMINAL_REACHABLE;
				}
				break;
			case PART_TERM:
				changed |= termReachabilitySimple(&grammarPart->uTerm);
				break;
			default:
				PANIC();
		}
	}
	return changed;
}

/* Note: the way we compute the contains sets might not be the most
   efficient, both in terms of memory use and time. However, it
   does make the procedure very readable, and the space and time required
   is not very great anyway. */
static bool nonTerminalComputeContainsSet(NonTerminal *nonTerminal);
static bool termComputeContainsSet(Term *term, bool fullContains, bool subtract);
static bool alternativeComputeContainsSet(Alternative *alternative);

/** Compute the contains sets for all @a NonTerminals.*/
void computeContainsSets(void) {
	transitiveClosure(nonTerminalComputeContainsSet);
}

/** Compute the contains set for a @a NonTerminal. */
static bool nonTerminalComputeContainsSet(NonTerminal *nonTerminal) {
	return termComputeContainsSet(&nonTerminal->term, true, nonTerminal->term.flags & TERM_NULLABLE);
}

/** Compute the contains set for a @a Term.
	@param term The @a Term to compute the contains set for.
	@param fullContains A boolean to indicate whether a full calculation
		should be done, or to simply use the @a Term's first set as its
		contains set. The latter is used for * repetitions without %persistent.
	@param subtract A boolean indicating whether the follow set should be
		subtracted from the contains set. This will only be done for @a Terms
		for which @a fullContains is also set. Subtraction should be performed
		for non-fixed repetitions and nullable @a Terms.
*/
static bool termComputeContainsSet(Term *term, bool fullContains, bool subtract) {
	bool changed = false;
	int i;
	Alternative *alternative;
	set oldSet;

	oldSet = newSet(condensedNumber);
	setCopy(oldSet, term->contains);

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		changed |= alternativeComputeContainsSet(alternative);
		if (fullContains) {
			if (alternative->flags & ALTERNATIVE_DEFAULT) {
				setUnion(term->contains, alternative->contains);
			} else {
				setUnion(term->contains, alternative->first);
			}
		}
	}
	if (fullContains && subtract)
		setMinus(term->contains, term->follow);
	setUnion(term->contains, term->first);

	changed |= !setEquals(oldSet, term->contains);
	deleteSet(oldSet);

	return changed;

}

/** Compute the contains set for an @a Alternative. */
static bool alternativeComputeContainsSet(Alternative *alternative) {
	bool changed = false;
	int i;
	GrammarPart *grammarPart;
	set oldSet;

	oldSet = newSet(condensedNumber);
	setCopy(oldSet, alternative->contains);

	for (i = listSize(alternative->parts) - 1; i >= 0; i--) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				break;
			case PART_NONTERMINAL:
				setUnion(alternative->contains, grammarPart->uNonTerminal.nonTerminal->term.contains);
				break;
			case PART_TERMINAL:
			case PART_LITERAL:
				setSet(alternative->contains, grammarPart->uTerminal.terminal);
				break;
			case PART_TERM:
				/* For everything but the non-persistent * repetitions, the
				   full calculations should be done. Subtraction should be
				   done for everything that has non-fixed repetition and for
				   fixed repetition with nullable alternatives. So sayeth the
				   authors of LLgen in their article/program. */
				changed |= termComputeContainsSet(&grammarPart->uTerm,
					(grammarPart->uTerm.flags & TERM_PERSISTENT) || (grammarPart->uTerm.repeats.subtype & (FIXED | PLUS)),
					grammarPart->uTerm.repeats.subtype != FIXED || (grammarPart->uTerm.flags & TERM_NULLABLE));
				setUnion(alternative->contains, grammarPart->uTerm.contains);
				break;
			default:
				PANIC();
		}
	}

	changed |= !setEquals(oldSet, alternative->contains);
	deleteSet(oldSet);
	return changed;
}

static bool nonTerminalComputeFinalReads(NonTerminal *nonTerminal);
static bool termComputeFinalReads(Term *term, bool force);
static bool alternativeComputeFinalReads(Alternative *alternative);

/** Determine which @a Terms necessarily end with a token read operation. */
void computeFinalReads(void) {
	transitiveClosure(nonTerminalComputeFinalReads);
}

/** Determine which of a @a NonTerminal's @a Terms necessarily end with a
		token read operation. */
static bool nonTerminalComputeFinalReads(NonTerminal *nonTerminal) {
	return termComputeFinalReads(&nonTerminal->term, false);
}

/** Determine if one of a @a Term's @a Alternatives necessarily end with a
		token read operation.
	@a term The @a Term to consider.
	@a force A boolean indicating that all this @a Term must end with a read.

	If one of a @a Term's @a Alternatives end in a read operation, all of its
	@a Alternatives should end with a read operation.

	Note: the @a Alternatives' no-final-read flags are not used during code
	generation. This means that we don't care what they are set to when the
	read is forced.
*/
static bool termComputeFinalReads(Term *term, bool force) {
	Alternative *alternative;
	bool changed = false;
	int i;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		changed |= alternativeComputeFinalReads(alternative);
	}

	/* Extra iterations are only necessary if the NonTerminals change.
	   Otherwise, nothing will change with the next iteration anyway.
	   Therefore we return false here. */
	if (!changed || (term->flags & TERM_NO_FINAL_READ))
		return false;

	if (force)
		return false;

	/* If one of the alternatives needs a final read, this Term does as well.
	   So, if we find one such alternative, we can just end (see the comment
	   above as well). */
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		if (!(alternative->flags & ALTERNATIVE_NO_FINAL_READ))
			return false;
	}
	term->flags |= TERM_NO_FINAL_READ;
	return true;
}

/** Determine if one an @a Alternative necessarily ends with a token read
		operation.

	Note: this also determines the final reads for the constituent parts.
*/
static bool alternativeComputeFinalReads(Alternative *alternative) {
	GrammarPart *grammarPart;
	int i;

	/* First determine all the final read flags for the inner terms */
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		if (grammarPart->subtype == PART_TERM)
			/* For all Terms that repeat, we need to force the read. */
			termComputeFinalReads(&grammarPart->uTerm, grammarPart->uTerm.repeats.subtype != FIXED || grammarPart->uTerm.repeats.number != 1);
	}

	if (alternative->flags & ALTERNATIVE_NO_FINAL_READ)
		return false;

	/* Then compute the final read flag of this alternative */
	for (i = listSize(alternative->parts) - 1; i >= 0; i--) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				break;
			case PART_NONTERMINAL:
				if (grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_NO_FINAL_READ) {
					alternative->flags |= ALTERNATIVE_NO_FINAL_READ;
					return true;
				}
				return false;
			case PART_TERMINAL:
			case PART_LITERAL:
				alternative->flags |= ALTERNATIVE_NO_FINAL_READ;
				return true;
			case PART_TERM:
				if (grammarPart->uTerm.flags & TERM_NO_FINAL_READ) {
					alternative->flags |= ALTERNATIVE_NO_FINAL_READ;
					return true;
				}
				return false;
			default:
				PANIC();
		}
	}
	return false;
}

static void resetNonTerminalReachable(NonTerminal *nonTerminal);

/** Reset the @a NONTERMINAL_REACHABLE flags for all @a NonTerminals. */
static void resetReachableForNonTerminals(void) {
	setOnlyReachable(false);
	walkRules(resetNonTerminalReachable);
}

/** Reset the @a NONTERMINAL_REACHABLE flags for a @a NonTerminals. */
static void resetNonTerminalReachable(NonTerminal *nonTerminal) {
	nonTerminal->flags &= ~NONTERMINAL_REACHABLE;
}

#ifdef USE_REGEX
static void termScanForUnidentifiedTokens(Term *term);
static void alternativeScanForUnidentifiedTokens(Alternative *alternative);

static bool needSemiColonEOL = false;

/** Scan non-terminals for yet undefined symbols that match the user defined pattern. */
void scanForUnidentifiedTokens(void) {
	walkRulesSimple(termScanForUnidentifiedTokens);
	if (option.dumpTokens) {
		if (needSemiColonEOL)
			puts(";");
		exit(EXIT_SUCCESS);
	}
}

/** Scan @a Terms for yet undefined symbols that match the user defined pattern. */
static void termScanForUnidentifiedTokens(Term *term) {
	int i;
	Alternative *alternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		alternativeScanForUnidentifiedTokens(alternative);
	}
}

/** Scan @a Alternatives for yet undefined symbols that match the user defined pattern. */
static void alternativeScanForUnidentifiedTokens(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;
	Declaration *declaration;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_UNKNOWN:
			case PART_ACTION:
			case PART_NONTERMINAL:
			case PART_TERMINAL:
			case PART_LITERAL:
				break;
			case PART_UNDETERMINED:
				declaration = (Declaration *) lookup(globalScope, grammarPart->token->text);
				if (declaration == NULL) {
					if (regexec(&option.tokenPattern, grammarPart->token->text, 0, NULL, 0) == 0) {
						/* This can be done in a nested call without the local variables,
						   but this way is cleaner */
						Directive *tokenDirective;
						Declaration *tokenDeclaration;

						tokenDirective = newDirective(TOKEN_DIRECTIVE, grammarPart->token, NULL);
						tokenDirective->uNumber = maxTokenNumber++;
						tokenDeclaration = newDeclaration(DIRECTIVE, tokenDirective);
						declare(globalScope, grammarPart->token->text, tokenDeclaration);

						if (option.dumpTokens) {
							switch (option.dumpTokensType) {
								case DUMP_TOKENS_MULTI: {
									static bool first = true;
									needSemiColonEOL = true;
									if (first) {
										printf("%%token %s", grammarPart->token->text);
										first = false;
									} else {
										printf(", %s", grammarPart->token->text);
									}
									break;
								}
								case DUMP_TOKENS_SEPARATE:
									printf("%%token %s;\n", grammarPart->token->text);
									break;
								case DUMP_TOKENS_LABELS: {
									char *label = grammarPart->token->text;

									if (option.lowercaseSymbols) {
										size_t j, length;
										label = safeStrdup(label, "alternativeScanForUnidentifiedTokens");
										length = strlen(label);
										for (j = 0; j < length; j++)
											label[j] = tolower(label[j]);
									}

									printf("%%label %s, \"%s\";\n", grammarPart->token->text, label);

									if (option.lowercaseSymbols)
										free(label);
									break;
								}
								default:
									PANIC();
							}
						}
					}
				}
				break;
			case PART_TERM:
				termScanForUnidentifiedTokens(&grammarPart->uTerm);
				break;
			default:
				PANIC();
		}
	}
}
#endif

static void termResolve(Term *term);
static void alternativeResolve(Alternative *alternative);

/** Resolve all references to (non-)terminals. */
void resolve(NonTerminal *nonTerminal) {
	termResolve(&nonTerminal->term);
}

/** Resolve all references to (non-)terminals. */
static void termResolve(Term *term) {
	int i;
	Alternative *alternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		alternativeResolve(alternative);
	}
}

/** Resolve all references to (non-)terminals.

	Note: this also checks the validity of character literals.
*/
static void alternativeResolve(Alternative *alternative) {
	GrammarPart *grammarPart;
	Declaration *declaration;
	int i, value;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_UNKNOWN:
			case PART_ACTION:
			case PART_TERMINAL:
				break;
			case PART_UNDETERMINED:
				declaration = (Declaration *) lookup(globalScope, grammarPart->token->text);
				if (declaration == NULL) {
					error(grammarPart->token, "Use of undeclared identifier '%s' in '%s'\n", grammarPart->token->text, getNonTerminalName(alternative->enclosing));
				} else {
					switch (declaration->subtype) {
						case DIRECTIVE:
							ASSERT(declaration->uDirective->subtype == TOKEN_DIRECTIVE);
							if (grammarPart->uNonTerminal.expression != NULL || grammarPart->uNonTerminal.retvalIdent != NULL) {
								error(grammarPart->token, "Trying to use terminal '%s' as non-terminal in '%s'\n",
									grammarPart->token->text, getNonTerminalName(alternative->enclosing));
								if (grammarPart->uNonTerminal.expression != NULL) {
									freeToken(grammarPart->uNonTerminal.expression);
									grammarPart->uNonTerminal.expression = NULL;
								}
								if (grammarPart->uNonTerminal.retvalIdent != NULL) {
									freeToken(grammarPart->uNonTerminal.retvalIdent);
									grammarPart->uNonTerminal.retvalIdent = NULL;
								}
							}
							grammarPart->subtype = PART_TERMINAL;
							grammarPart->uTerminal.terminal = terminalToCondensed[declaration->uDirective->uNumber];
							if (grammarPart->uTerminal.terminal == 0)
								warning(WARNING_EOFILE, grammarPart->token, "Use of EOFILE token requires special care. See documentation for details\n");
							break;
						case NONTERMINAL:
							grammarPart->subtype = PART_NONTERMINAL;
							grammarPart->uNonTerminal.nonTerminal = declaration->uNonTerminal;

							if (!option.noArgCount) {
								grammarPart->uNonTerminal.argCount = determineArgumentCount(grammarPart->uNonTerminal.expression, false);
								if (grammarPart->uNonTerminal.argCount != grammarPart->uNonTerminal.nonTerminal->argCount)
									softError(grammarPart->token, "Parameter count mismatch in call of non-terminal '%s' in '%s'\n", grammarPart->token->text, getNonTerminalName(alternative->enclosing));
							}

							if (grammarPart->uNonTerminal.retvalIdent != NULL) {
								Declaration *globalDeclaration;
								Token *retvalIdent;

								retvalIdent = grammarPart->uNonTerminal.retvalIdent;
								globalDeclaration = lookup(globalScope, retvalIdent->text);
								if (globalDeclaration != NULL && globalDeclaration->subtype == DIRECTIVE && globalDeclaration->uDirective->subtype == TOKEN_DIRECTIVE) {
									softError(retvalIdent, "Cannot rename return value to '%s' because a token with the same name exists\n", retvalIdent->text);
								} else if (checkClash(retvalIdent->text, SEL_NONE)) {
									softError(retvalIdent, "Return value name '%s' clashes with generated symbol\n", retvalIdent->text);
								}
							}
							break;
						default:
							PANIC();
					}
				}
				break;
			case PART_LITERAL:
				/* checkLiteral will output any necessary messages */
				value = checkLiteral(grammarPart->token);
				if (value > 0 || (option.noEOFZero && value == 0)) {
					if (terminalToCondensed[value] == -1)
						terminalToCondensed[value] = condensedNumber++;
					grammarPart->uTerminal.terminal = terminalToCondensed[value];
				}
				break;
			case PART_TERM:
				termResolve(&grammarPart->uTerm);
				if (grammarPart->uTerm.repeats.subtype & FINOPT) {
					if (i+1 != listSize(alternative->parts))
						error(grammarPart->token, "Optional-final repetition only allowed on last item of an alternative\n");
					if (alternative->enclosing->repeats.number == 1) {
						warning(WARNING_UNMASKED, grammarPart->token, "Enclosing term for optional-final repetition does not repeat\n");
						grammarPart->uTerm.repeats.subtype = STAR;
						/* repeats.number is already 1 */
					}
				}
				/* Previous code may have changed the repetition type to STAR, so
				   retest before setting TERM_CONTAINS_FINOPT flag. */
				if (grammarPart->uTerm.repeats.subtype & FINOPT) {
					alternative->enclosing->flags |= TERM_CONTAINS_FINOPT;
					alternative->flags |= ALTERNATIVE_CONTAINS_FINOPT;
				}
				break;
			case PART_BACKREF: {
				Alternative *outerAlternative;
				int itemCount, j;

				if (alternative->enclosing->index == THREAD_NONTERMINAL) {
					error(grammarPart->token, "Back-reference operator may only be used in a term\n");
					break;
				}

				outerAlternative = alternative->enclosing->enclosing.alternative;
				itemCount = alternative->enclosing->index;
				if (itemCount == 0) {
					error(grammarPart->token, "Back-reference operator refers to nothing\n");
					break;
				}

				/* Remove the ... operator ... */
				listDelete(alternative->parts, i);
				/* ... and replace with what it refers to. */
				for (j = itemCount - 1; j >= 0; j--) {
					GrammarPart *copy = copyGrammarPart(listIndex(outerAlternative->parts, j), grammarPart->token, alternative);
					listInsert(alternative->parts, i, copy);
				}

				/* Correct index member of any terms from where the ... was. */
				for (j = i; j < listSize(alternative->parts); j++) {
					GrammarPart *followingPart = listIndex(alternative->parts, j);
					if (followingPart->subtype == PART_TERM)
						followingPart->uTerm.index = j;
				}

				/* Clean up memory used by the ... operator. */
				freeToken(grammarPart->token);
				free(grammarPart);
				/* Correct i, because we have inserted parts */
				i += itemCount - 1;
				break;
			}
			default:
				PANIC();
		}
	}
}

static void nonTerminalSetAttributes(NonTerminal *nonTerminal);
static void termSetAttributes(Term *term);
static void alternativeSetAttributes(Alternative *alternative);

static int nonTerminalNumber = 0;

/** Set attributes required for further analysis and code generation. */
void setAttributes(void) {
	/* Attributes set here are:
		TERM_OPTMULTREP
		TERM_MULTIPLE_ALTERNATIVE
		NONTERMINAL_GLOBAL
	*/
	walkRules(nonTerminalSetAttributes);
}

/** Set attributes required for further analysis and code generation. */
static void nonTerminalSetAttributes(NonTerminal *nonTerminal) {
	nonTerminal->number = nonTerminalNumber++;
	termSetAttributes(&nonTerminal->term);
}

/** Set attributes required for further analysis and code generation. */
static void termSetAttributes(Term *term) {
	int i;
	Alternative *alternative;

	/* This info is useful, because we don't merge single alternatives into
	   the enclosing alternative. The rationale is that I want to keep the
	   information about conflict resolvers, so we can bug the user. We could
	   also do that when merging it in, but my philosophy is that we should
	   try to keep the error messages grouped as well as possible. */
 	if (listSize(term->rule) > 1)
		term->flags |= TERM_MULTIPLE_ALTERNATIVE;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		alternativeSetAttributes(alternative);
	}
}

/** Set attributes required for further analysis and code generation. */
static void alternativeSetAttributes(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
			case PART_TERMINAL:
			case PART_LITERAL:
				break;
			case PART_NONTERMINAL:
				/* Note: no need for strcmp, as all fileNames from the same
				   file point to the same string. */
				if (option.LLgenStyleOutputs && grammarPart->token->fileName != grammarPart->uNonTerminal.nonTerminal->token->fileName)
					grammarPart->uNonTerminal.nonTerminal->flags |= NONTERMINAL_GLOBAL;
				break;
			case PART_TERM:
				/* If this term may repeat here, set the flag */
				if (grammarPart->uTerm.repeats.number < 0 || grammarPart->uTerm.repeats.number > 1)
					grammarPart->uTerm.flags |= TERM_OPTMULTREP;
				termSetAttributes(&grammarPart->uTerm);
				break;
			case PART_UNDETERMINED:
			default:
				PANIC();
		}
	}
}

static void nonTerminalAllocateSets(NonTerminal *term);
static void termAllocateSets(Term *term);
static void alternativeAllocateSets(Alternative *alternative);

/** Allocate the sets for the @a Terms and @a Alternatives. */
void allocateSets(void) {
	walkRules(nonTerminalAllocateSets);
}


/** Allocate the sets for a @a NonTerminal.

	This also already sets the EOFILE token in the follow sets of those
	non-terminals for which a %start directive has been specified.
*/
static void nonTerminalAllocateSets(NonTerminal *nonTerminal) {
	termAllocateSets(&nonTerminal->term);
	if (nonTerminal->flags & NONTERMINAL_START)
		setSet(nonTerminal->term.follow, 0);
}

/** Allocate the sets for a @a Term. */
static void termAllocateSets(Term *term) {
	int i;
	Alternative *alternative;

	term->first = newSet(condensedNumber);
	term->follow = newSet(condensedNumber);
	term->contains = newSet(condensedNumber);

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		alternativeAllocateSets(alternative);
	}
}

/** Allocate the sets for an @a Alternative. */
static void alternativeAllocateSets(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;

	alternative->first = newSet(condensedNumber);
	alternative->contains = newSet(condensedNumber);

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
			case PART_NONTERMINAL:
			case PART_TERMINAL:
			case PART_LITERAL:
				break;
			case PART_TERM:
				termAllocateSets(&grammarPart->uTerm);
				break;
			default:
				PANIC();
		}
	}
}

static void termFixupFirst(Term *term);
static void alternativeFixupFirst(Alternative *alternative);

/** Fixup the @a Alternative first sets, with information from the enclosing
		@a Term.

	We do this here, because we need the follow set information to complement
	the first set information. As the follow set information is not available
	at first set calculation, this needs to be done afterwards.
*/
void fixupFirst(void) {
	walkRulesSimple(termFixupFirst);
}

/** Fixup the first sets of all a @a Term's @a Alternatives. */
static void termFixupFirst(Term *term) {
	int i;
	Alternative *alternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		/* For nullable alternatives, the Terms follow set is the first set. */
		if (alternative->flags & ALTERNATIVE_NULLABLE) {
			setUnion(alternative->first, term->follow);
			/* This is sure to cause an alternation conflict, but
			   there should be one in this case because this alternative
			   can be chosen when the term repeats. However, when it
			   repeats, it means that all the other alternatives may
			   follow this one. Therefore it will always cause a conflict. */
			if (term->flags & TERM_OPTMULTREP)
				setUnion(alternative->first, term->first);
		}
		alternativeFixupFirst(alternative);
	}
}

/** Call @a termFixupFirst for all @a Terms in an @a Alternative. */
static void alternativeFixupFirst(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		if (grammarPart->subtype == PART_TERM)
			termFixupFirst(&grammarPart->uTerm);
	}
}

static void nonTerminalCollectReturnValues(NonTerminal *nonTerminal);
static void termCollectReturnValues(Term *term);
static void alternativeCollectReturnValues(Alternative *alternative);

/** Collect all the return values used in the rules and list them in the
	@a NonTerminal they are used in. */
void collectReturnValues(void) {
	walkRules(nonTerminalCollectReturnValues);
}

/** Ugly hack to avoid passing the pointer to the current retvalScope to all
	..CollectReturnValues functions. */
static Scope *currentRetvalScope;

/** Collect all names of the variables in which return values will be stored.
	@param nonTerminal the @a NonTerminal to check.
*/
static void nonTerminalCollectReturnValues(NonTerminal *nonTerminal) {
	nonTerminal->retvalScope = newScope();
	currentRetvalScope = nonTerminal->retvalScope;
	termCollectReturnValues(&nonTerminal->term);
	currentRetvalScope = NULL; /* Make sure we can't do silly things. */
}

/** Collect all names of the variables in which return values will be stored.
	@param term the @a Term to check.
*/
static void termCollectReturnValues(Term *term) {
	int i;
	Alternative *alternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		alternativeCollectReturnValues(alternative);
	}
}

/** Collect all names of the variables in which return values will be stored.
	@param alternative the @a Alternative to check.
*/
static void alternativeCollectReturnValues(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_NONTERMINAL:
				if (grammarPart->uNonTerminal.nonTerminal->retvalIdent != NULL) {
					const char *key;
					/* Any key named LLretval or LLdiscard is NOT stored in the scope, because
					   it needs separate treatment when generating code, and specialised
					   checking is done anyway. */
					if (!(grammarPart->uNonTerminal.flags & (CALL_LLRETVAL | CALL_LLDISCARD))) {
						key = grammarPart->uNonTerminal.retvalIdent == NULL ?
							grammarPart->token->text : grammarPart->uNonTerminal.retvalIdent->text;
						declare(currentRetvalScope, key, grammarPart);
					}
				}
				break;
			case PART_TERM:
				termCollectReturnValues(&grammarPart->uTerm);
				break;
			case PART_ACTION:
			case PART_TERMINAL:
			case PART_LITERAL:
				break;
			default:
				PANIC();
		}
	}
}

static unsigned alternativeMarkDiscardedReturn(Alternative *alternative, unsigned discardFlags);
static unsigned termMarkDiscardedReturn(Term *term, unsigned discardFlags);

/** Mark @a NonTerminal calls whose return value is/might be ignored. */
static void nonTerminalMarkDiscardedReturn(NonTerminal *nonTerminal) {
	termMarkDiscardedReturn(&nonTerminal->term, CALL_DISCARDS_RETVAL);
}

/** Mark @a NonTerminal calls whose return value is/might be ignored in a @a Term.
	@param term The @a Term to mark.
	@param discardFlags The flags to be used for calls to mark.
	@return The new discard flags to be used for any preceeding calls.
*/
static unsigned termMarkDiscardedReturn(Term *term, unsigned discardFlags) {
	Alternative *alternative;
	unsigned returnedFlags, newDiscardFlags = 0;
	int noDiscardCount = 0;
	int i;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		returnedFlags = alternativeMarkDiscardedReturn(alternative, discardFlags);
		if (returnedFlags & CALL_MAY_DISCARD_RETVAL)
			newDiscardFlags = returnedFlags;
		else if (returnedFlags == 0)
			noDiscardCount++;
	}

	if (noDiscardCount == 0)
		newDiscardFlags |= CALL_DISCARDS_RETVAL;
	else if (noDiscardCount != i)
		newDiscardFlags = CALL_MAY_DISCARD_RETVAL | CALL_DISCARDS_RETVAL;
	return newDiscardFlags;
}

/** Mark @a NonTerminal calls whose return value is/might be ignored in an @a Alternative.
	@param alternative The @a Alternative to mark.
	@param discardFlags The flags to be used for calls to mark.
	@return The new discard flags to be used for any preceeding calls.
*/
static unsigned alternativeMarkDiscardedReturn(Alternative *alternative, unsigned discardFlags) {
	GrammarPart *grammarPart;
	int i;

	for (i = listSize(alternative->parts) - 1; i >= 0; i--) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_NONTERMINAL:
				if (grammarPart->uNonTerminal.nonTerminal->retvalIdent != NULL) {
					if (grammarPart->uNonTerminal.retvalIdent == NULL ||
							!(grammarPart->uNonTerminal.flags & (CALL_LLRETVAL | CALL_LLDISCARD)))
						grammarPart->uNonTerminal.flags |= discardFlags;
				}

				if (option.noArgCount) {
					if (grammarPart->uNonTerminal.expression != NULL)
						return 0;
				} else {
					if (grammarPart->uNonTerminal.argCount > 0)
						return 0;
				}

				break;
			case PART_TERM: {
				unsigned returnedFlags = termMarkDiscardedReturn(&grammarPart->uTerm, discardFlags);
				if (returnedFlags == 0) {
					/* If the term itself contains code, but it may not be executed, then we
					   consider all preceding rules that return a value as "may discard". If
					   it is always executed, we're done. */
					if (TERM_REPEATS_NULLABLE(grammarPart))
						returnedFlags = CALL_MAY_DISCARD_RETVAL | CALL_DISCARDS_RETVAL;
					else
						return 0;
				}
				discardFlags = returnedFlags;
				break;
			}
			case PART_ACTION:
				return 0;
			case PART_TERMINAL:
			case PART_LITERAL:
				break;
			default:
				PANIC();
		}
	}
	return discardFlags;
}

static void nonTerminalPrintConflicts(NonTerminal *nonTerminal);
static void termMarkConflicts(Term *term);
static void termPrintConflicts(Term *term);
static void alternativeMarkConflicts(Alternative *alternative);
static void alternativePrintConflicts(Alternative *alternative);

/** Find all conflicts in all @a NonTerminals.

	This is a two-phase process. First we mark all the conflicts, then we
	write error messages for them. This is to ensure that we write the error
	and warning messages in order (as much as is practical).
*/
void findConflicts(void) {
	Declaration *declaration;
	int i;

	resetReachableForNonTerminals();
	walkRules(nonTerminalReachabilityInit);
	transitiveClosure(nonTerminalReachability);
	setOnlyReachable(true);
	walkRules(nonTerminalMarkDiscardedReturn);
	walkRulesSimple(termMarkConflicts);

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == NONTERMINAL) {
			currentRetvalScope = declaration->uNonTerminal->retvalScope;
			nonTerminalPrintConflicts(declaration->uNonTerminal);
		} else if (declaration->subtype == DIRECTIVE) {
			Directive *directive = declaration->uDirective;
			if (directive->subtype == TOKEN_DIRECTIVE &&
					!(condensedToTerminal[terminalToCondensed[directive->uNumber]].flags & CONDENSED_REACHABLE))
				warning(WARNING_UNUSED, directive->token[0], "Token '%s' is not used\n", directive->token[0]->text);
		}
	}
}

/** Print error and warning messages for a @a NonTerminal.

	NOTE: this is not called from @a walkRules, and therefore we need to check
	for @a onlyReachable ourselves!
*/
static void nonTerminalPrintConflicts(NonTerminal *nonTerminal) {
	/* Issue a warning when a non-terminal is neither reachable nor mentioned
	   in a %first directive. */
	if (!(nonTerminal->flags & (NONTERMINAL_REACHABLE | NONTERMINAL_FIRST))) {
		warning(WARNING_UNUSED, nonTerminal->token, "Non-terminal '%s' is not used\n", nonTerminal->token->text);
		if (onlyReachable)
			return;
	}
	/* Don't do further searching for conflicts if the NonTerminal is
	   unreachable and onlyReachable is set. */
	if (!(nonTerminal->flags & NONTERMINAL_REACHABLE) && onlyReachable)
		return;
	if (!option.noArgCount && (nonTerminal->flags & NONTERMINAL_START) && nonTerminal->argCount != 0)
		error(nonTerminal->token, "Rule specified in a %%start directive ('%s') cannot have arguments\n", nonTerminal->token->text);

	if (nonTerminal->flags & NONTERMINAL_RECURSIVE_DEFAULT)
		error(nonTerminal->token, "Recursion in default for non-terminal '%s'\n", nonTerminal->token->text);
	termPrintConflicts(&nonTerminal->term);
}

/** Mark conflicts found in a @a Term. */
static void termMarkConflicts(Term *term) {
	int i, j;
	Alternative *alternative, *otherAlternative;
	set prefer;

	prefer = newSet(condensedNumber);

	/* IMPROVEMENT?: We ought to mark the alternation conflict here, but
		we also want to specify with which other alternatives the
		conflict is in our error messages. This seems to conflict
		and result into multiple checking of the alternation conflict.
		NOTE: if we mark the term as having a conflict, than we can
		skip the duplicate checking for all those that don't!!
	*/
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		/* This alternative cannot start on anything for which an
		   alternative marked with %prefer exists. */
		for (j = 0; j < i; j++) {
			otherAlternative = (Alternative *) listIndex(term->rule, j);
			/* Do intersection test on alternatives that don't yet have the
			   ALTERNATIVE_ACONFLICT flag set */
			if (!(otherAlternative->flags & ALTERNATIVE_ACONFLICT) && !setIntersectionEmpty(alternative->first, otherAlternative->first))
				otherAlternative->flags |= ALTERNATIVE_ACONFLICT;
		}
		setMinus(alternative->first, prefer);
		if (alternative->flags & ALTERNATIVE_PREFER)
			/* If necessary add this alternative's first set to the %prefer set */
			setUnion(prefer, alternative->first);
		alternativeMarkConflicts(alternative);
	}
	/* At this point it is safe to remove the other first sets for the %avoid
	   marked alternatives. We couldn't do it before because otherwise the
	   conflicts wouldn't get marked and we would produce warnings about
	   conflict resolvers without conflict. */
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		if (alternative->flags & ALTERNATIVE_AVOID) {
			/* Remove the other first sets from this alternative's first set*/
			for (j = i + 1; j < listSize(term->rule); j++) {
				otherAlternative = (Alternative *) listIndex(term->rule, j);
				setMinus(alternative->first, otherAlternative->first);
			}
		}
		if (setEmpty(alternative->first) && listSize(term->rule) > 1)
			alternative->flags |= ALTERNATIVE_NEVER_CHOSEN;
	}
	deleteSet(prefer);
}

/** Test two @a Alternatives for an alternation conflict and print an
		appropriate message.
	@param alternative The @a Alternative to test.
	@param previousAlternative The @a Alternative to test against.
*/
static void testAlternationConflict(Alternative *alternative, Alternative *previousAlternative) {
	if (!setIntersectionEmpty(alternative->first, previousAlternative->first)) {
		if (!(previousAlternative->flags & ALTERNATIVE_CONDITION)) {
			error(alternative->token, "Alternation conflict with alternative at line %d in '%s'\n", previousAlternative->token->lineNumber, getNonTerminalName(alternative->enclosing));
			if (option.verbose > 0) {
				set conflicts;
				conflicts = newSet(condensedNumber);
				setCopy(conflicts, alternative->first);
				setIntersection(conflicts, previousAlternative->first);

				if ((alternative->flags & ALTERNATIVE_NULLABLE) && (previousAlternative->flags & ALTERNATIVE_NULLABLE)) {
					fprintf(stderr, "Both alternatives can produce empty. Omitting tokens from follow set.\n");
					setMinus(conflicts, alternative->enclosing->follow);
					if (setEmpty(conflicts)) {
						deleteSet(conflicts);
						return;
					}
				}

				fprintf(stderr, "Trace for the conflicting tokens from alternative on line %d:\n", previousAlternative->token->lineNumber);
				traceAlternativeTokens(previousAlternative, conflicts);
				fprintf(stderr, "Trace for the conflicting tokens from alternative on line %d:\n", alternative->token->lineNumber);
				traceAlternativeTokens(alternative, conflicts);
				deleteSet(conflicts);
			}
		}
	}
}

/* IMPROVEMENT?: Perhaps we could improve on LLgen by
also checking for %if(0) and %if(1), although there are many other ways to fool
checking. The only way it will happen is if the other alternatives together have
in their aggregated FIRST set the complete FIRST set of the alternative with %avoid.
*/
/** Print error and warning messages for a @a Term. */
static void termPrintConflicts(Term *term) {
	bool hasDefault = false;
	int i, j;
	Alternative *alternative = NULL, *previousAlternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		if (alternative->flags & ALTERNATIVE_DEFAULT) {
			if (hasDefault)
				error(alternative->token, "More than one alternation with %%default\n");
			hasDefault = true;
		}
		if ((alternative->flags & ALTERNATIVE_CONDITION) && !(alternative->flags & ALTERNATIVE_ACONFLICT))
			warning(WARNING_UNMASKED, alternative->token, "Conflict resolver without conflict in '%s'\n", getNonTerminalName(alternative->enclosing));
		for (j = 0; j < i; j++) {
			previousAlternative = (Alternative *) listIndex(term->rule, j);
			/* Only check when a previous alternative has a conflict */
			if (previousAlternative->flags & ALTERNATIVE_ACONFLICT)
				testAlternationConflict(alternative, previousAlternative);
		}
		if (alternative->flags & ALTERNATIVE_NEVER_CHOSEN)
			error(alternative->token, "Alternative never chosen in '%s'\n", getNonTerminalName(alternative->enclosing));
		if (alternative->flags & ALTERNATIVE_CONDITION && i == listSize(term->rule) - 1)
			error(alternative->token, "Cannot specify a condition on last alternative in '%s'\n", getNonTerminalName(alternative->enclosing));
		alternativePrintConflicts(alternative);
	}
}

/** Mark conflicts found in an @a Alternative. */
static void alternativeMarkConflicts(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;


	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		if (grammarPart->subtype == PART_TERM) {
			termMarkConflicts(&grammarPart->uTerm);
			/* If the term MAY repeat more than once or MAY be skipped ('?'), then check for
			   repetition conflicts */
			if ((grammarPart->uTerm.repeats.subtype & (STAR|PLUS|FINOPT)) &&
					!setIntersectionEmpty(grammarPart->uTerm.first, grammarPart->uTerm.follow)) {
				grammarPart->uTerm.flags |= TERM_RCONFLICT;
				if ((grammarPart->uTerm.flags & TERM_CONTAINS_FINOPT) &&
						(grammarPart->uTerm.repeats.subtype & (STAR|PLUS))) {
					/* No need to check for repeats.number != 1, because in that case
					   TERM_CONTAINS_FINOPT is not set (see alternativeResolve). */
					grammarPart->uTerm.flags |= TERM_REQUIRES_LLCHECKED;
				}
			}
		}
	}
}

static bool compareRetvalTypes(Token *a, Token *b) {
	return strcmp(a->text, b->text) == 0;
}

/** Print error and warning messages for an @a Alternative. */
static void alternativePrintConflicts(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				break;
			case PART_NONTERMINAL:
				if (grammarPart->uNonTerminal.nonTerminal->retvalIdent == NULL) {
					if (grammarPart->uNonTerminal.retvalIdent != NULL)
						error(grammarPart->uNonTerminal.retvalIdent, "Return value renamed for non-terminal '%s' which does not return a value\n", grammarPart->token->text);
				} else {
					GrammarPart *firstUse;
					const char *key;

					/* Any key named LLretval is NOT stored in the scope. Specialised
					   checking is done here. */
					if (grammarPart->uNonTerminal.flags & CALL_LLRETVAL) {
						NonTerminal *currentNonTerminal = getEnclosingNonTerminal(alternative->enclosing);
						if (currentNonTerminal->retvalIdent == NULL)
							error(grammarPart->uNonTerminal.retvalIdent, "Cannot assign value to LLretval because '%s' does not return a value\n", currentNonTerminal->token->text);
						else if (!compareRetvalTypes(grammarPart->uNonTerminal.nonTerminal->retvalIdent, getEnclosingNonTerminal(alternative->enclosing)->retvalIdent))
							error(grammarPart->uNonTerminal.retvalIdent, "Return value of '%s' is incompatible with return value for '%s'\n",
								grammarPart->token->text, getNonTerminalName(alternative->enclosing));
						break;
					} else if (grammarPart->uNonTerminal.flags & CALL_LLDISCARD) {
						break;
					}

					key = grammarPart->uNonTerminal.retvalIdent == NULL ?
						grammarPart->token->text : grammarPart->uNonTerminal.retvalIdent->text;
					firstUse = lookup(currentRetvalScope, key);
					ASSERT(firstUse != NULL);
					if (!compareRetvalTypes(grammarPart->uNonTerminal.nonTerminal->retvalIdent, firstUse->uNonTerminal.nonTerminal->retvalIdent)) {
						if (grammarPart->uNonTerminal.retvalIdent == NULL) {
							error(grammarPart->token, "Use of return value of '%s' has different type than earlier renamed value of '%s'\n",
								grammarPart->token->text, firstUse->token->text);
						} else {
							error(grammarPart->token, "Cannot rename value of '%s' to '%s' because '%s' has already been used with different type at ",
								grammarPart->token->text, grammarPart->uNonTerminal.retvalIdent->text, grammarPart->uNonTerminal.retvalIdent->text);
							printAt(firstUse->token);
							endMessage();
						}
					}
				}

				if (grammarPart->uNonTerminal.flags & CALL_DISCARDS_RETVAL)
					warning(WARNING_DISCARD_RETVAL, grammarPart->token, "Return value of non-terminal '%s' %s used (rename value to LLdiscard if this intended)\n",
						grammarPart->uNonTerminal.nonTerminal->token->text,
						grammarPart->uNonTerminal.flags & CALL_MAY_DISCARD_RETVAL ? "may not be" : "is not");

				break;
			case PART_TERMINAL:
			case PART_LITERAL:
				break;
			case PART_TERM:
				if (grammarPart->uTerm.flags & TERM_WHILE) {
					if (grammarPart->uTerm.repeats.subtype == FIXED)
						error(grammarPart->token, "%%while not allowed in this term in '%s'\n", getNonTerminalName(alternative->enclosing));
					else if (!(grammarPart->uTerm.flags & TERM_RCONFLICT))
						warning(WARNING_UNMASKED, grammarPart->token, "%%while without a conflict in '%s'\n", getNonTerminalName(alternative->enclosing));
				} else if (grammarPart->uTerm.flags & TERM_RCONFLICT) {
					error(grammarPart->token, "Repetition conflict in '%s'\n", getNonTerminalName(&grammarPart->uTerm));
					if (option.verbose > 0)
						traceTermRepetitionConflict(&grammarPart->uTerm);
				}

				if ((grammarPart->uTerm.repeats.subtype & (STAR | PLUS | FINOPT)) && grammarPart->uTerm.flags & TERM_NULLABLE)
					error(grammarPart->token, "Term with variable repetition can produce empty in '%s'\n", getNonTerminalName(alternative->enclosing));

				termPrintConflicts(&grammarPart->uTerm);
				break;
			case PART_UNDETERMINED:
			default:
				PANIC();
		}
	}
}

static void termSetDefault(Term *term);
static void alternativeSetDefault(Alternative *alternative);
/** Choose which @a Alternatives are the default choices and set flags. */
void setDefaults(void) {
	walkRulesSimple(termSetDefault);
}

/** Set the default choice for a @a Term. */
static void termSetDefault(Term *term) {
	int i, min;
	Alternative *alternative, *shortestAlternative = NULL;

	if (listSize(term->rule) == 0)
		return;
	/* Set the default flags for all embedded Terms. */
	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		alternativeSetDefault(alternative);
	}

	/* Iterate over all Alternatives to find the shortest. */
	shortestAlternative = (Alternative *) listIndex(term->rule, 0);
	/* If the user has specified a %default, use that. */
	if (shortestAlternative->flags & ALTERNATIVE_DEFAULT)
		return;

	min = shortestAlternative->length;

	for (i = 1; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		/* If the user has specified a %default, use that. */
		if (alternative->flags & ALTERNATIVE_DEFAULT) {
			shortestAlternative = alternative;
			break;
		} else if (alternative->length < min) {
			/* Go for the shortest alternative to be the default. */
			shortestAlternative = alternative;
			min = alternative->length;
		} else if (alternative->length == min && (shortestAlternative->flags & ALTERNATIVE_AVOID) && !(alternative->flags & ALTERNATIVE_AVOID)) {
			/* If the previously chosen one is as long as this one, and the
			   previously chosen one has the %avoid directive specified and
			   this alternative doesn't: switch to this alternative as the
			   default. */
			shortestAlternative = alternative;
		}
	}
	shortestAlternative->flags |= ALTERNATIVE_DEFAULT;
}

/** Set the default choice for all @a Terms embedded in an @a Alternative. */
static void alternativeSetDefault(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		if (grammarPart->subtype == PART_TERM)
				termSetDefault(&grammarPart->uTerm);
	}
}

static void nonTerminalEnumerateSets(NonTerminal *nonTerminal);
static void termEnumerateSets(Term *term);
static void alternativeEnumerateSets(Alternative *alternative);

/** Enumerate all the sets used in all the @a NonTerminals and %first macros. */
void enumerateSets(void) {
	/* We only need sets for NonTerminals for which code is actually generated.
	   For %first macros the enumerateMacroSets function is used. The analysis
	   above however needed to take the NonTerminals that were referenced by
	   %first macros into account as if they were reachable because it needs
	   to calculate the first set for these NonTerminals correctly. */
	resetReachableForNonTerminals();
	walkRules(nonTerminalCodeReachabilityInit);
	transitiveClosure(nonTerminalReachability);
	setOnlyReachable(true);
	walkRules(nonTerminalEnumerateSets);
	walkDirectives(enumerateMacroSets);
}

/** Enumerate all the sets used in a @a NonTerminal. */
static void nonTerminalEnumerateSets(NonTerminal *nonTerminal) {
	if ((nonTerminal->term.flags & TERM_MULTIPLE_ALTERNATIVE) && setFill(nonTerminal->term.contains) > 1)
		setFindIndex(nonTerminal->term.contains, true);
	termEnumerateSets(&nonTerminal->term);
}

/** Enumerate all the sets used in a @a Term's @a Alternatives. */
static void termEnumerateSets(Term *term) {
	int i;
	Alternative *alternative;

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		alternativeEnumerateSets(alternative);
	}
}

/** Enumerate all the sets used in all an @a Alternative's @a Terms. */
static void alternativeEnumerateSets(Alternative *alternative) {
	int i;
	GrammarPart *grammarPart;
	bool needsPush = false;

	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_NONTERMINAL:
				/* Trying to enumerate the same set twice (see
				   nonTerminalEnumerateSets) makes no difference. */
				if (needsPush && setFill(grammarPart->uNonTerminal.nonTerminal->term.contains) > 1)
					setFindIndex(grammarPart->uNonTerminal.nonTerminal->term.contains, true);
				break;
			case PART_ACTION:
			case PART_TERMINAL:
			case PART_LITERAL:
				break;
			case PART_TERM:
				if (setFill(grammarPart->uTerm.contains) > 1)
					setFindIndex(grammarPart->uTerm.contains, true);
				if (grammarPart->uTerm.repeats.subtype & PLUS && !(grammarPart->uTerm.flags & TERM_PERSISTENT) && setFill(grammarPart->uTerm.first) > 1)
					setFindIndex(grammarPart->uTerm.first, true);
				termEnumerateSets(&grammarPart->uTerm);
				break;
			default:
				PANIC();
		}
		if (!needsPush && grammarPart->subtype != PART_ACTION)
			needsPush = true;
	}
}

