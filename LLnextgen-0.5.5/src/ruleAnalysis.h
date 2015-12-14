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

#ifndef RULE_H
#define RULE_H

typedef struct NonTerminal NonTerminal;
typedef struct Alternative Alternative;

#include "nonRuleAnalysis.h"
#include "list.h"
#include "set.h"

#include "posixregex.h"

/* Repeats are clasified as a bitfield to make TERM_REPEATS_NULLABLE evaluate
   its argument only once
*/
typedef enum {
	FIXED = (1<<0),
	STAR = (1<<1),
	PLUS = (1<<2),
	FINOPT = (1<<3)
} RepeatsSubtype;

#define TERM_REPEATS_NULLABLE(x) ((x)->uTerm.repeats.subtype & (STAR | FINOPT))

typedef struct {
	RepeatsSubtype subtype;
	int number;	/* -1 for none supplied */
} Repeats;


#define THREAD_NONTERMINAL (-1)

#define TERM_PERSISTENT (1U<<0)
#define TERM_WHILE (1U<<1)
#define TERM_NULLABLE (1U<<2)	/* Set ONLY if the term itself can produce NULL, not when
								   it is followed by a nullable repetition operator */
#define TERM_OPTMULTREP (1U<<3)	/* Set if a term can optionally repeat more than once
								   For non-terminals this means that in SOME rule they
								   can optionally repeat more than once. */
#define TERM_MULTIPLE_ALTERNATIVE (1U<<4)
#define TERM_RCONFLICT (1U<<5)
#define TERM_NO_FINAL_READ (1U<<6)
#define TERM_CONTAINS_FINOPT (1U<<7)
#define TERM_REQUIRES_LLCHECKED (1U<<8)

typedef struct {
	unsigned flags;
	int length;

	List *rule; /* List of Alternatives pointers */
	set first, follow, contains;

	Repeats repeats;
	CCode *expression;	/* %while expression */
	
	/* Members for tracing conflicts in the tree */
	union {
		Alternative *alternative;
		NonTerminal *nonTerminal;
	} enclosing;
	int index;		/* Index in the alternative or THREAD_NONTERMINAL */

	set *conflicts;
} Term;

#define NONTERMINAL_START (1U<<0)
		/* This NonTerminal is mentioned in a %start directive. */
#define NONTERMINAL_REACHABLE (1U<<1)
		/* Do we need to generate code for this non-terminal? */
#define NONTERMINAL_RECURSIVE_DEFAULT (1U<<2)
		/* Is the default production recursive? */
#define NONTERMINAL_GLOBAL (1U<<3)
		/* Is the non-terminal referenced from outside the file it was defined
		   in. This should only be calculated if multiple files are to be
		   generated. */
#define NONTERMINAL_VISITED (1U<<4)
#define NONTERMINAL_TRACING (1U<<5)
		/* Flags set by the trace routines to prevent recursion in the rules
		   inducing an infinite recursion in LLnextgen. Visited is set when
		   tracing into an alternative, traced is set when tracing calls to
		   the non-terminal. */
#define NONTERMINAL_TRACED (1U<<6)
		/* Flag set by the trace routines to prevent multiple printing of the
		   same sub rules in conflicts. This mainly occurs in expression
		   constructs where precedence is implemented by splitting out the
		   different precedence levels into separate rules. */
#define NONTERMINAL_FIRST (1U<<7)
		/* This NonTerminal is mentioned in a %first directive, and therefore
		   unused non-terminal messages should be suppressed. */

struct NonTerminal {
	Term term;
	CCode *parameters, *declarations;
	Token *token, *retvalIdent;
	Scope *retvalScope;
	int argCount;
	int number;
	unsigned flags;
};

NonTerminal *newNonTerminal(Token *token, Token *retIdent, CCode *parameters, CCode *localDeclarations, List *rule, int argCount);

#define ALTERNATIVE_IF (1<<0)
#define ALTERNATIVE_DEFAULT (1<<1)
#define ALTERNATIVE_PREFER (1<<2)
#define ALTERNATIVE_AVOID (1<<3)
#define ALTERNATIVE_CONDITION (ALTERNATIVE_IF | ALTERNATIVE_PREFER | ALTERNATIVE_AVOID)
#define ALTERNATIVE_ACONFLICT (1<<4)
#define ALTERNATIVE_NULLABLE (1<<5)
#define ALTERNATIVE_NEVER_CHOSEN (1<<6)
#define ALTERNATIVE_NO_FINAL_READ (1<<7) 
	/* Flag IS NOT and SHOULD NOT be used for code generation, only to compute
	   the TERM_NO_FINAL_READ flag! */
#define ALTERNATIVE_CONTAINS_FINOPT (1<<8)
	/* Only used for tracing tokens. */

struct Alternative {
	int flags;
	List *parts; /* List of GrammarPart pointers */
	Token *token;
	CCode *expression; /* Expression for %if */
	set first;
	set contains;
	int length;
	
	/* Label to jump to, to reach the code for this alternative. */
	int label;				

	Term *enclosing;
#ifdef MEM_DEBUG
	bool dontFreeToken;
#endif
};

Alternative *newAlternative(void);

/* Parts are clasified as a bitfield to make PART_IS_TERMINAL evaluate
	its argument only once. Other implementations of PART_IS_TERMINAL
	that only evalute their argument once are less portible (i.e. uses typeof).
*/
typedef enum {
	PART_UNKNOWN = (1<<0),		/* Part created by parser error handling. */
	PART_ACTION = (1<<1),
	PART_NONTERMINAL = (1<<2),
	PART_TERMINAL = (1<<3),		/* %token defined */
	PART_LITERAL = (1<<4),		/* character literal */
	PART_UNDETERMINED = (1<<5),	/* Token or non-terminal, lookup has to determine */
	PART_TERM = (1<<6),			/* Something between [] */
	PART_BACKREF = (1<<7)
} GrammarPartSubtype;
#define PART_IS_TERMINAL(x) ((*(x)).subtype & (PART_LITERAL | PART_TERMINAL))

#define CALL_DISCARDS_RETVAL (1U<<0)
#define CALL_MAY_DISCARD_RETVAL (1U<<1) /* If set, CALL_DISCARDS_RETVAL will also be set */
#define CALL_LLRETVAL (1U<<2)
#define CALL_LLDISCARD (1U<<3)

typedef struct {
	GrammarPartSubtype subtype;
	Token *token;
	union {
		Term term;
		
		struct {
			NonTerminal *nonTerminal;	/* The non-terminal that is called */
			CCode *expression;			/* Parameters */
			Token *retvalIdent;
			int argCount;
			unsigned flags;		/* Flags of the call, not the called non-terminal */
		} nonTerminal;
		
		CCode *action;		/* Action */
		
		struct {
			int terminal;		/* Terminal */
		} terminal;
	} un;
#ifdef MEM_DEBUG
	bool dontFreeToken;
#endif
} GrammarPart;

GrammarPart *newGrammarPart(Token *token, GrammarPartSubtype subtype);
GrammarPart *wrapInTerm(GrammarPart *grammarPart);

NonTerminal *getEnclosingNonTerminal(Term *term);
#define getNonTerminalName(x) (getEnclosingNonTerminal(x)->token->text)

void setOnlyReachable(bool newValue);

void determineNullability(void);
void computeFirstSets(void);
void computeFollowSets(void);
void computeLengths(void);
void computeContainsSets(void);
void computeFinalReads(void);
void determineReachabilitySimple(void);
#ifdef USE_REGEX
void scanForUnidentifiedTokens(void);
#endif
void resolve(NonTerminal *nonTerminal);
void allocateSets(void);
void fixupFirst(void);
void collectReturnValues(void);
void setAttributes(void);
void findConflicts(void);
void setDefaults(void);
void enumerateSets(void);

void walkRules(void (*nonTerminalAction)(NonTerminal *));
void walkRulesSimple(void (*termAction)(Term *));
#endif
