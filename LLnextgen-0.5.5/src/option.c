/* Copyright (C) 2005,2006,2008,2009 G.P. Halkes
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
#include <stdlib.h>
#include <limits.h>

#include "posixregex.h"

#include "optionMacros.h"
#include "option.h"
#include "globals.h"
#include "os.h"
#include "generate.h"

options option;


#define OPTIONS_BASE (1<<0)
#define OPTIONS_EXTRA (1<<1)
#define OPTIONS_HELP (1<<2)
#define OPTIONS_DEPEND (1<<3)
#define OPTIONS_ERROR (1<<4)
#define OPTIONS_BASE_ONLY (1<<5)

typedef struct {
	const char *string;
	int when;
} optionDescription;

optionDescription descriptions[] = {
{ "    -c, --max-compatibility      Set options required for maximum compatibility\n", OPTIONS_EXTRA },
{ "                                 This implies --llgen-arg-style,\n", OPTIONS_EXTRA },
{ "                                 --llgen-escapes-only and --llgen-output-style\n", OPTIONS_EXTRA },
{ "    -e, --warnings-as-errors     Treat warnings as errors\n", OPTIONS_ERROR },
{ "    -Enum, --error-limit=num     Set the maximum number of errors\n", OPTIONS_ERROR },
{ "    -h[which], --help[=which]    Print out this help message. [which] can be\n", OPTIONS_BASE | OPTIONS_HELP },
{ "                                 all, depend, error, extra\n", OPTIONS_BASE | OPTIONS_HELP },
{ "    -V, --version                Print the program version and exit\n", OPTIONS_BASE },
{ "    -v[level], --verbose[=level] Increase or set the verbosity level\n", OPTIONS_BASE | OPTIONS_ERROR},
{ "    -w[warnings], --suppress-warnings[=warnings]\n", OPTIONS_BASE | OPTIONS_ERROR },
{ "                                 Suppress all or selected warnings. \n", OPTIONS_BASE | OPTIONS_ERROR },
{ "                                 See --help=error or man page for more details\n", OPTIONS_BASE_ONLY },
{ "                                 Available warnings are: arg-separator,\n", OPTIONS_ERROR },
{ "                                 option-override, unbalanced-c, multiple-parser,\n", OPTIONS_ERROR },
{ "                                 eofile, unused[:<identifier>], datatype, and\n", OPTIONS_ERROR },
{ "                                 unused-retval\n", OPTIONS_ERROR },
{ "    --abort                      Generate the LLabort function\n", OPTIONS_EXTRA },
{ "    --base-name=name             Set the base name for the output files\n", OPTIONS_EXTRA },
{ "    --extensions=list            Set the extensions for the output files\n", OPTIONS_EXTRA },
{ "    --depend[=modifiers]         Generate dependency information. Modifiers can\n", OPTIONS_DEPEND },
{ "                                 be used to change what is output. Available\n", OPTIONS_DEPEND },
{ "                                 modifiers are: targets:<targets>, file:<file>\n", OPTIONS_DEPEND },
{ "                                 extra-targets:<targets> and phony\n", OPTIONS_DEPEND },
{ "    --depend-cpp                 Extract code to pass through the\n", OPTIONS_DEPEND },
{ "                                 C preproccessor for dependency information\n", OPTIONS_DEPEND },
{ "    --dump-lexer-wrapper         Dump the lexer wrapper function to screen\n", OPTIONS_BASE },
{ "    --dump-llmessage             Dump the default LLmessage function to screen\n", OPTIONS_BASE },
#ifdef USE_REGEX
{ "    --dump-tokens[=modifier]     Dump %token directives for unknown identifiers\n", OPTIONS_EXTRA },
{ "                                 that match the --token-pattern pattern\n", OPTIONS_BASE },
{ "                                 Available modifiers: separate, labels\n", OPTIONS_BASE },
#endif
{ "    --generate-lexer-wrapper[=yes|no]\n", OPTIONS_BASE },
{ "                                 Generate a wrapper for the lexical analyser\n", OPTIONS_BASE },
{ "    --generate-llmessage         Generate an LLmessage function\n", OPTIONS_BASE },
{ "    --generate-symbol-table      Generate a symbol table\n", OPTIONS_BASE },
{ "    --gettext[=macro,guard]      Surround labels with gettext macro\n", OPTIONS_EXTRA },
{ "    --keep-dir                   Create outputs in same directory as inputs\n", OPTIONS_EXTRA },
{ "    --llgen-arg-style            Use semicolons as argument separator\n", OPTIONS_EXTRA },
{ "    --llgen-escapes-only         Only use LLgen character-literal escapes\n", OPTIONS_EXTRA },
{ "    --llgen-output-style         Generate Lpars.[ch] and one .c file per input\n", OPTIONS_EXTRA },
{ "    --lowercase-symbols          Convert names to lowercase in symbol table\n", OPTIONS_BASE },
{ "    --no-allow-label-create      %label won't to create new tokens\n", OPTIONS_EXTRA },
{ "    --no-arg-count               Do not check argument counts\n", OPTIONS_EXTRA },
{ "    --no-eof-zero                Do not use 0 as end-of-file token\n", OPTIONS_BASE },
{ "    --no-init-llretval           Do not initialize LLretval to 0 bytes\n", OPTIONS_EXTRA },
{ "    --no-line-directives         Do not generate #line directives in the output\n", OPTIONS_EXTRA },
{ "    --no-llreissue               Do not generate the LLreissue variable\n", OPTIONS_EXTRA },
{ "    --no-prototypes-header       Do not generate a prototypes in the .h file\n", OPTIONS_EXTRA },
{ "    --not-only-reachable         Do not only analyse reachable rules\n", OPTIONS_EXTRA },
{ "    --reentrant                  Generate a reentrant parser (NOT thread-safe!)\n", OPTIONS_BASE },
{ "    --show-dir                   Show directory names in messages\n", OPTIONS_ERROR },
{ "    --thread-safe                Generate a thread safe parser\n", OPTIONS_BASE },
#ifdef USE_REGEX
{ "    --token-pattern=pattern      Pattern for matching unknown identifiers\n", OPTIONS_EXTRA },
#endif
{ NULL, 0 } };

optionDescription helpArgs[] = {
{ "all", ~(OPTIONS_BASE_ONLY) },
{ "depend", OPTIONS_DEPEND },
{ "error", OPTIONS_ERROR },
{ "extra", OPTIONS_EXTRA },
{ "help", OPTIONS_HELP },
{ "which", OPTIONS_HELP },
{ NULL, 0 }
};

static void checkDependArguments(const char *optArg);

/** Parse command line arguments.
	@param argc The number of arguments on the command line.
	@param argv The array of character pointers containing the command line
		arguments.
*/
PARSE_FUNCTION(parseCmdLine)
	memset(&option, 0, sizeof(option));
	option.inputFileList = newList();
	option.errorLimit = -1;

	/* Check whether the program was called through the LLgen symlink */
	if (isLLgen(argv[0])) {
		option.llgenMode = true;
		option.LLgenArgStyle = true;
		option.onlyLLgenEscapes = true;
		option.LLgenStyleOutputs = true;
		option.noLLreissue = true;
		option.reentrant = true;
	}

	/* In LLgen mode we should only accept the original LLgen options. In
	   LLnextgen mode we can remove the original options. The -h option is
	   then the same as --help in LLnextgen mode */
	OPTIONS
		/* ============= Options from LLgen ============= */
		if (option.llgenMode) {
			SHORT_OPTION('a', NO_ARG) /* LLgen: Generate ANSI compatible output. Ignored */
			END_OPTION
			SHORT_OPTION('h', REQUIRED_ARG) /* LLgen: Set high_percentage for jump table. Ignored */
				int discard;
				PARSE_INT(discard, 0, 100);
			END_OPTION
			SHORT_OPTION('j', OPTIONAL_ARG) /* LLgen: Generate dense switches. Ignored */
				if (optArg != NULL) {
					int discard;
					PARSE_INT(discard, 0, 100);
				}
			END_OPTION
			SHORT_OPTION('l', REQUIRED_ARG) /* LLgen: Set low_percentage for jump table. Ignored */
				int discard;
				PARSE_INT(discard, 0, 100);
			END_OPTION
			SHORT_OPTION('n', OPTIONAL_ARG);
				fatal("LLnextgen cannot generate parsers with non-correcting error-recovery\n");
			END_OPTION
			SHORT_OPTION('s', OPTIONAL_ARG);
				fatal("LLnextgen cannot generate parsers with non-correcting error-recovery\n");
			END_OPTION
			SHORT_OPTION('x', NO_ARG) /* LLgen: Generate extended sets. Ignored */
				fprintf(stderr, "LLnextgen cannot generate extended sets\n");
				break;
			END_OPTION
			SHORT_OPTION('v', NO_ARG)
				option.verbose++;
				/* Prevent overflow */
				if (option.verbose > MAX_VERBOSITY)
					option.verbose = MAX_VERBOSITY;
			END_OPTION
			BOOLEAN_SHORT_OPTION('w', option.suppressWarnings)
		} else {

			OPTION('v', "verbose", OPTIONAL_ARG)
				option.verboseSet = true;
				if (optArg != NULL) {
					PARSE_INT(option.verbose, 1, MAX_VERBOSITY);
				} else {
					option.verbose++;
					/* Prevent overflow */
					if (option.verbose > MAX_VERBOSITY)
						option.verbose = MAX_VERBOSITY;
				}
			END_OPTION
			OPTION('w', "suppress-warnings", OPTIONAL_ARG)
				checkWarningSuppressionArguments(optArg, (int) optlength, CURRENT_OPTION);
			END_OPTION
			/* ============= New options ============= */
			OPTION('V', "version", NO_ARG)
				fputs("LLnextgen " VERSION_STRING "\nCopyright (C) 2005-2011 G.P. Halkes\nLicensed under the GNU General Public License version 3\n", stdout);
				exit(0);
			END_OPTION
			OPTION('c', "max-compatibility", NO_ARG)
				option.LLgenArgStyle = true;
				option.onlyLLgenEscapes = true;
				option.LLgenStyleOutputs = true;
			END_OPTION
			BOOLEAN_OPTION('e', "warnings-as-errors", option.warningsErrors)
			LONG_OPTION("base-name", REQUIRED_ARG)
				if (option.outputBaseNameSet)
					free((char *) option.outputBaseName);
				else
					option.outputBaseNameSet = true;
				option.outputBaseName = safeStrdup(optArg, "parseCmdLine");
			END_OPTION
			LONG_OPTION("extensions", REQUIRED_ARG)
				if (option.extensions != NULL)
					deleteListWithContents(option.extensions);
				option.extensions = split(optArg, ",", true);
			END_OPTION
			BOOLEAN_LONG_OPTION("llgen-arg-style", option.LLgenArgStyle)
			BOOLEAN_LONG_OPTION("llgen-escapes-only", option.onlyLLgenEscapes)
			BOOLEAN_LONG_OPTION("keep-dir", option.keepDirInFilename)
			BOOLEAN_LONG_OPTION("no-prototypes-header", option.noPrototypesHeader)
			BOOLEAN_LONG_OPTION("no-line-directives", option.dontGenerateLineDirectives)
			BOOLEAN_LONG_OPTION("llgen-output-style", option.LLgenStyleOutputs)
			BOOLEAN_LONG_OPTION("reentrant", option.reentrant)
			BOOLEAN_LONG_OPTION("no-eof-zero", option.noEOFZero)
			BOOLEAN_LONG_OPTION("no-init-llretval", option.noInitLLretval)
			BOOLEAN_LONG_OPTION("no-llreissue", option.noLLreissue)
			LONG_OPTION("generate-lexer-wrapper", OPTIONAL_ARG)
				option.generateLexerWrapperSpecified = true;
				if (optArg == NULL) {
					option.generateLexerWrapper = true;
				} else {
					if (strcmp(optArg, "no") == 0)
						option.generateLexerWrapper = false;
					else if (strcmp(optArg, "yes") == 0)
						option.generateLexerWrapper = true;
					else
						fatal("Argument '%s' is invalid for option --generate-lexer-wrapper\n", optArg);
				}
			END_OPTION
			BOOLEAN_LONG_OPTION("generate-llmessage", option.generateLLmessage)
			BOOLEAN_LONG_OPTION("generate-symbol-table", option.generateSymbolTable)
			BOOLEAN_LONG_OPTION("abort", option.abort)
			BOOLEAN_LONG_OPTION("no-allow-label-create", option.noAllowLabelCreate)
			BOOLEAN_LONG_OPTION("dump-lexer-wrapper", option.dumpLexerwrapper)
			BOOLEAN_LONG_OPTION("dump-llmessage", option.dumpLLmessage);
			OPTION('h', "help", OPTIONAL_ARG)
				int i, when = OPTIONS_BASE | OPTIONS_BASE_ONLY;
				printf("Usage: LLnextgen [OPTIONS] [FILES]\n");

				if (optArg != NULL) {
					for (i = 0; helpArgs[i].string != NULL; i++) {
						if (strcmp(helpArgs[i].string, optArg) == 0) {
							when = helpArgs[i].when;
							break;
						}
					}

					if (helpArgs[i].string == NULL) {
						printf("    Argument to %.*s is unsupported. Showing help for %.*s.\n", (int) optlength, CURRENT_OPTION, (int) optlength, CURRENT_OPTION);
						when = OPTIONS_HELP;
					}
				}

				for (i = 0; descriptions[i].string != NULL; i++) {
					if (descriptions[i].when & when)
						fputs(descriptions[i].string, stdout);
				}
				exit(EXIT_SUCCESS);
			END_OPTION
			OPTION('E', "error-limit", REQUIRED_ARG)
				option.errorLimitSet = true;
				PARSE_INT(option.errorLimit, 0, INT_MAX);
			END_OPTION
			BOOLEAN_LONG_OPTION("show-dir", option.showDir);
			BOOLEAN_LONG_OPTION("no-arg-count", option.noArgCount);
			BOOLEAN_LONG_OPTION("lowercase-symbols", option.lowercaseSymbols);
			BOOLEAN_LONG_OPTION("not-only-reachable", option.notOnlyReachable);
			LONG_OPTION("depend", OPTIONAL_ARG)
				option.depend = true;
				checkDependArguments(optArg);
			END_OPTION
			BOOLEAN_LONG_OPTION("depend-cpp", option.dependCpp)
			BOOLEAN_LONG_OPTION("thread-safe", option.threadSafe)
#ifdef USE_REGEX
			LONG_OPTION("token-pattern", REQUIRED_ARG)
				int result;

				option.useTokenPattern = true;
				if ((result = regcomp(&option.tokenPattern, optArg, REG_EXTENDED | REG_NOSUB)) != 0) {
					char errorBuffer[100];
					regerror(result, &option.tokenPattern, errorBuffer, 100);
					fatal("Regular expression could not be compiled: %s\n", errorBuffer);
				}
			END_OPTION
			LONG_OPTION("dump-tokens", OPTIONAL_ARG)
				option.dumpTokens = true;
				if (optArg != NULL) {
					if (strcmp(optArg, "separate") == 0)
						option.dumpTokensType = DUMP_TOKENS_SEPARATE;
					else if (strcmp(optArg, "labels") == 0)
						option.dumpTokensType = DUMP_TOKENS_LABELS;
					/* Next is the default, so we don't include it in the docs,
					   but we allow it anyway. */
					else if (strcmp(optArg, "multiple") == 0)
						option.dumpTokensType = DUMP_TOKENS_MULTI;
					else
						fatal("Argument '%s' is invalid for option --dump-tokens\n", optArg);
				}
			END_OPTION
#else
			LONG_OPTION("token-pattern", OPTIONAL_ARG)
				fatal("This LLnextgen binary does not support the %.*s option. See the documentation for details\n", (int) optlength, CURRENT_OPTION);
			END_OPTION
			LONG_OPTION("dump-tokens", OPTIONAL_ARG)
				fatal("This LLnextgen binary does not support the %.*s option. See the documentation for details\n", (int) optlength, CURRENT_OPTION);
			END_OPTION
#endif
			LONG_OPTION("gettext", OPTIONAL_ARG)
				checkGettextArguments(optArg);
			END_OPTION
		}
		fatal("Option %.*s does not exist\n", (int) optlength, CURRENT_OPTION);
	NO_OPTION
		listAppend(option.inputFileList, safeStrdup(CURRENT_OPTION, "parseCmdLine"));
	END_OPTIONS

	if (option.dumpLexerwrapper) {
		fputs(option.threadSafe ? "int lexerWrapper(struct <prefix>this LLthis) {\n" : "int lexerWrapper(void) {\n", stdout);
		printf(lexerWrapperString, "LL_NEW_TOKEN", "<lexer>", option.threadSafe ? "LLthis" : "", "LL_NEW_TOKEN");
		fputs("\nWhere <lexer> should be replaced with the name of the lexical analyzer routine", stdout);
		if (option.threadSafe)
			fputs(",\n and <prefix> by LL or the prefix selected with %prefix", stdout);
		fputs(".\nDon't forget to add \"%lexical lexerWrapper;\" to your grammar!\n", stdout);
	}

	if (option.dumpLLmessage) {
		if (option.dumpLexerwrapper)
			fputc('\n', stdout);
		fputs(option.threadSafe ? "void <prefix>message(struct <prefix>this LLthis, int LLtoken) {\n" : "void <prefix>message(int LLtoken) {\n", stdout);
		fputs(llmessageString[0], stdout);
		fputs(llmessageString[1], stdout);
		fputs("\nWhere <prefix> should be replaced by LL or the prefix selected with %prefix\nDon't forget to add --generate-symbol-table to your options!\n", stdout);
	}

	if (option.dumpLexerwrapper || option.dumpLLmessage)
		exit(EXIT_SUCCESS);

	postOptionChecks();
	if (option.LLgenStyleOutputs && listSize(option.inputFileList) == 0)
		fatal("Cannot generate output from standard input %s\n", option.llgenMode ? "in LLgen mode" : "with --llgen-output-style");

	if (option.depend && option.dependCpp)
		fatal("Cannot generate dependency information and output C-preprocessor code at the same time\n");

	if (listSize(option.inputFileList) == 0 && option.outputBaseName == NULL)
		option.outputBaseName = "LLparser";
END_FUNCTION

/** Check the consistency of the options supplied by the user.

	This functions makes sure that the options supplied by the user are
	consistent, and either emits a warning or aborts with an error. Errors are
	only issued when files would be generated in a different place if the
	options had been consistent.
*/
void postOptionChecks(void) {
	if (option.llgenMode && option.verbose)
		fprintf(stderr, "This is LLnextgen, running as LLgen. Turning on compatibility options.\n");

	if (option.generateLexerWrapper && option.noLLreissue) {
		fprintf(stderr, "warning: Option --no-llreissue cannot be used with --generate-lexer-wrapper\n");
		option.noLLreissue = false;
	}

	/* This can be done, but gives a hassle in generateLLgenStyle.c. As LLgen
	   can't do it either, we don't allow it. Note that simply ignoring these
	   options would result in different files being generated, and therefore
	   errors instead of warnings are used. */
	if (option.LLgenStyleOutputs && option.outputBaseNameSet)
		fatal("Cannot use --base-name %s\n", option.llgenMode ? "in LLgen mode" : "with --llgen-output-style");
	if (option.LLgenStyleOutputs && option.keepDirInFilename)
		fatal("Cannot use --keep-dir %s\n", option.llgenMode ? "in LLgen mode" : "with --llgen-output-style");

	if (option.threadSafe && option.reentrant)
		fatal("Cannot use --reentrant and --thread-safe at the same time\n");

	if (option.threadSafe && option.LLgenStyleOutputs)
		fatal("Cannot use --llgen-output-style with --thread-safe\n");

	if (option.threadSafe && option.noArgCount)
		fatal("Cannot use --no-arg-count for thread-safe parsers.\n");

	if (option.generateLLmessage)
		option.generateSymbolTable = true;

	if (option.warningsErrors && option.suppressWarnings) {
		static bool warned = false; /* Needed to suppress duplicate warnings */
		if (!warned) {
			fprintf(stderr, "warning: Using both --warnings-as-errors and --suppress-warnings makes no sense\n");
			warned = true;
		}
	}

	if (option.extensions != NULL && listSize(option.extensions) > 2)
		fatal("Only 2 extensions may be specified (source,header).\n");
}

/** Check the arguments for the --suppress-warnings option.
	@param optArg The arguments supplied for the option.
	@param optlength The length of @a current_option.
	@param current_option The current option (to allow correct printing of -W and --suppress-warnings)
*/
void checkWarningSuppressionArguments(const char *optArg, int optlength, const char *current_option) {
	if (optArg == NULL) {
		option.suppressWarnings = true;
	} else {
		char *subarg, *strtokState;
		/* Make a copy, for those C implementations in which argv[*] is not
		   writable */
		char *optargCopy = safeStrdup(optArg, "checkWarningSuppressionArguments");
		subarg = strtokReentrant(optargCopy, ",", &strtokState);
		if (subarg == NULL) {
			fprintf(stderr, "warning: argument for option %.*s is empty\n", optlength, current_option);
		}
		while (subarg != NULL) {
			if (strcmp(subarg, "arg-separator") == 0) {
				option.suppressWarningsTypes |= WARNING_ARG_SEPARATOR;
			} else if (strcmp(subarg, "option-override") == 0) {
				option.suppressWarningsTypes |= WARNING_OPTION_OVERRIDE;
			} else if (strcmp(subarg, "unbalanced-c") == 0) {
				option.suppressWarningsTypes |= WARNING_UNBALANCED_C;
			} else if (strcmp(subarg, "multiple-parser") == 0) {
				option.suppressWarningsTypes |= WARNING_MULTIPLE_PARSER;
			} else if (strcmp(subarg, "eofile") == 0) {
				option.suppressWarningsTypes |= WARNING_EOFILE;
			} else if (strcmp(subarg, "unused") == 0) {
				option.suppressWarningsTypes |= WARNING_UNUSED;
			} else if (strcmp(subarg, "reentrant-wrapper") == 0) {
				fprintf(stderr, "The reentrant-wrapper suppression is deprecated\n");
			} else if (strcmp(subarg, "datatype") == 0) {
				option.suppressWarningsTypes |= WARNING_DATATYPE;
			} else if (strcmp(subarg, "unused-retval") == 0) {
				option.suppressWarningsTypes |= WARNING_DISCARD_RETVAL;
			} else if (strncmp(subarg, "unused:", 7) == 0) {
				if (strlen(subarg) > 7) {
					if (option.unusedIdentifiers == NULL)
						option.unusedIdentifiers = newList();
					listAppend(option.unusedIdentifiers, safeStrdup(subarg + 7, "checkWarningSuppressionArguments"));
				} else {
					fprintf(stderr, "warning: argument 'unused:' for option %.*s needs an identifier\n", optlength, current_option);
				}
			} else {
				fatal("Argument '%s' is invalid for option %.*s\n", subarg, optlength, current_option);
			}
			subarg = strtokReentrant(NULL, ",", &strtokState);
		}
		free(optargCopy);
	}
}

/** Check the arguments for the --depend option.
	@param optArg The arguments supplied for the option.
*/
static void checkDependArguments(const char *optArg) {
	if (optArg == NULL) {
		option.suppressWarnings = true;
	} else {
		char *subarg, *strtokState;
		/* Make a copy, for those C implementations in which argv[*] is not
		   writable */
		char *optargCopy = safeStrdup(optArg, "checkDependArguments");
		subarg = strtokReentrant(optargCopy, ",", &strtokState);
		if (subarg == NULL) {
			fprintf(stderr, "warning: argument for option --depend is empty\n");
		}
		while (subarg != NULL) {
			if (strncmp(subarg, "targets:", 8) == 0) {
				if (strlen(subarg) > 7) {
					option.dependTargets = safeStrdup(subarg + 8, "checkDependArguments");
				} else {
					fprintf(stderr, "warning: argument 'targets:' for option --depend needs text\n");
				}
			} else if (strncmp(subarg, "file:", 5) == 0) {
				if (strlen(subarg) > 7) {
					option.dependFile = safeStrdup(subarg + 5, "checkDependArguments");
				} else {
					fprintf(stderr, "warning: argument 'file:' for option --depend needs file name\n");
				}
			} else if (strncmp(subarg, "extra-targets:", 14) == 0) {
				if (strlen(subarg) > 7) {
					option.dependExtraTargets = safeStrdup(subarg + 14, "checkDependArguments");
				} else {
					fprintf(stderr, "warning: argument 'extra-targets:' for option --depend needs text\n");
				}
			} else if (strcmp(subarg, "phony") == 0) {
				option.dependPhony = true;
			} else {
				fatal("Argument '%s' is invalid for option --depend\n", subarg);
			}
			subarg = strtokReentrant(NULL, ",", &strtokState);
		}
		free(optargCopy);
	}
}

/** Check if a string contains only acceptable C-identifier characters.
	@param string The string to check
	@return A boolean indicating whether all characters are acceptable.
*/
static bool isAcceptable(const char *string) {
	return strspn(string, "_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") == strlen(string);
}

/** Check the arguments for the --gettext option
	@param optArg The arguments supplied for the option.
*/
void checkGettextArguments(const char *optArg) {
	option.gettext = true;
	if (optArg == NULL) {
		option.gettextMacro = "N_";
		option.gettextGuard = "USE_NLS";
	} else {
		char *comma;
		option.gettextMacro = safeStrdup(optArg, "checkGettextArguments");
		option.gettextGuard = comma = strchr(option.gettextMacro, ',');
		if (option.gettextGuard == NULL) {
			option.gettextGuard = "USE_NLS";
		} else {
			*comma = 0;
			option.gettextGuard++;
			if (strlen(option.gettextGuard) == 0)
				option.gettextGuard = "USE_NLS";
		}

		if (strlen(option.gettextMacro) == 0)
			option.gettextMacro = "N_";

		if (!isAcceptable(option.gettextMacro))
			fatal("gettext macro name contains unacceptable character(s)\n");
		if (!isAcceptable(option.gettextGuard))
			fatal("gettext guard name contains unacceptable character(s)\n");
	}
}
