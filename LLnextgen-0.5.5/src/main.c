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

#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "option.h"
#include "generate.h"
#include "printInput.h"
#include "lexer.h"
#include "depend.h"

#define LL_NOTOKENS
#include "grammar.h"

/*
Main steps in the main routine:
- parse command line
- call parser();
- determine if there have been analysis stopping errors
	true -> exit
- set up the scope
	- put all tokens in the scope, and determine number of tokens
	- add all non-terminals to the scope
- determine if there have been analysis stopping errors
	true -> exit
- resolve the non-terminals specified in all grammar parts
- determine if there have been analysis stopping errors
	true -> exit
- allocate first/follows/contains sets
- determine nullability
- do analysis
	- calculate all first sets, including the ones for alternatives
		and terms
	- calculate all necessary follow sets, including the ones for terms
- determine if there have been any errors so far
	true -> exit
- calculate contains sets
- calculate remaining attributes required for code generation
- generate code
*/

int main(int argc, char *argv[]) {
	parseCmdLine(argc, argv);

	openFirstInput();
	do {
		parser();
	} while (nextFile());

	if (option.dependCpp)
		dependCpp();
	else if (option.depend)
		depend();

	if (option.gettext && !option.generateSymbolTable)
		generalWarning(WARNING_UNMASKED, "Using --gettext only makes sense with --generate-symbol-table\n");

	if (!option.generateLexerWrapperSpecified)
		generalWarning(WARNING_UNMASKED, "The --generate-lexer-wrapper option has not been specified. If you do\n    not want an automatically generated wrapper for your lexical analyser, use\n    --generate-lexer-wrapper=no. See documentation for details.\n");

	setupScope(); /* No errors are reported here. */

	setupResolve(); /* No errors can occur here */
	checkDeclarations();
	if (errorSeen) {
		freeMemory();
		fatal("Input is ambiguous; aborting.\n");
	}

	fixCondensedTable();
	
	/* As the processing of the %start directive expects the first sets to be
	   valid, we allocate them first. A small optimisation could be to first
	   only allocate the first sets for the non-terminals themselves, and
	   postpone the rest to after the reachability has been determined. */
	allocateSets();
	
	/* Figure out which non-terminals are unreachable and should be excluded
	   from further analysis. Note that the final "unreachable" messages are
	   not generated based on this analysis, but the analysis started in
	   findConflicts. */
	determineReachabilitySimple();
	setOnlyReachable(true);
	
	determineNullability();
	setAttributes();
	
	computeFirstSets();
	computeFollowSets();
	fixupFirst();

	/* Compute the minimum lengths of Term's and Alternatives. Also sets the
	   NONTERMINAL_RECURSIVE_DEFAULT flag. This seems counter intuitive, but
	   the only way a default can be recursive is if the %default directive
	   has been specified. Therefore we don't yet have to have set the
	   defaults to set the NONTERMINAL_RECURSIVE_DEFAULT flag. */
	computeLengths();
	collectReturnValues();
	findConflicts();

	/* These depend only on the sets as computed previously. I'd rather do
	   them after checking for errors, but we need them for printing. */
	setDefaults();
	computeContainsSets();

	if (option.verbose >= 2)
		printDeclarations();

	if (errorSeen || softErrorSeen) {
		freeMemory();
		exit(EXIT_FAILURE);
	}

	/* Can't go wrong, because any problems have been detected previously */
	computeFinalReads();
	enumerateSets();
	
	if (option.LLgenStyleOutputs)
		generateLLgenStyle();
	else if (option.threadSafe)
		generateTS();
	else
		generate();

	freeMemory();
	exit(EXIT_SUCCESS);
}
