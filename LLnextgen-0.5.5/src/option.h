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

#ifndef OPTION_H
#define OPTION_H

#include "bool.h"
#include "list.h"
#include "nonRuleAnalysis.h"

#define MAX_VERBOSITY 3

#include "posixregex.h"

#ifdef USE_REGEX
typedef enum {
	DUMP_TOKENS_MULTI = 0,
	DUMP_TOKENS_SEPARATE,
	DUMP_TOKENS_LABELS
} DumpTokensTypes;
#endif

typedef struct {
	/* This flag cannot be set by any command line option, but is set
	   automatically when the program is started by the name LLgen. */
	bool llgenMode;
	
	int verbose;
	bool verboseSet;
	
	/* suppressWarnings suppresses all warnings, suppressWarningsTypes is a
	   bitmap of specific warning options, and unusedIdentifiers is a list of
	   identifiers for which the unused warning is to be suppressed. */
	bool suppressWarnings;
	int suppressWarningsTypes;
	List *unusedIdentifiers;
	
	bool showDir;
	
	bool LLgenArgStyle;
	bool onlyLLgenEscapes;
	
	bool warningsErrors;
	
	const char *outputBaseName;
	Token *outputBaseNameLocation;
	bool outputBaseNameSet;

	List *extensions;
	Token *extensionsLocation;

	List *inputFileList;
	bool keepDirInFilename;
	bool noPrototypesHeader;
	bool dontGenerateLineDirectives;
	bool LLgenStyleOutputs;
	bool reentrant;
	bool noEOFZero;
	bool noLLreissue;
	bool generateLexerWrapperSpecified;
	Token *generateLexerWrapperLocation;
	bool generateLexerWrapper;
	bool generateLLmessage;
	bool abort;
	bool generateSymbolTable;
	bool noAllowLabelCreate;
	bool noArgCount;
	bool notOnlyReachable;

	int errorLimit;
	bool errorLimitSet;
	
	bool lowercaseSymbols;
	
	bool depend;
	const char *dependTargets;
	const char *dependExtraTargets;
	const char *dependFile;
	bool dependPhony;

	bool dependCpp;
	
#ifdef USE_REGEX
	bool useTokenPattern;
	regex_t tokenPattern;
	bool dumpTokens;
	DumpTokensTypes dumpTokensType;
#endif

	bool dumpLLmessage;
	bool dumpLexerwrapper;

	bool threadSafe;

	bool gettext;
	const char *gettextMacro, *gettextGuard;

	bool noInitLLretval;
} options;

extern options option;
void parseCmdLine(int argc, char *argv[]);
void postOptionChecks(void);
void checkWarningSuppressionArguments(const char *optarg, int optlength, const char *current_option);
void checkGettextArguments(const char *optarg);
#endif
