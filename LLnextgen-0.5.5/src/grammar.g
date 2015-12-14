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

%options "generate-symbol-table generate-lexer-wrapper=no lowercase-symbols reentrant no-eof-zero";

%lexical lexerWrapper;
{  /* start of C code */
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include "posixregex.h"

#include "nonRuleAnalysis.h"
#include "ruleAnalysis.h"
#include "globals.h"
#include "argcount.h"
#include "option.h"
#include "localOptionMacros.h"
#include "lexer.h"
#include "bool.h"

/** Function required by parser to show error messages.
	@param symb A symbol to be inserted or a value indicating the deletion of
		tokens.
*/
void LLmessage(int symb) {
	switch (symb) {
		case LL_MISSINGEOF:
			error(NULL, "Unexpected %s", LLgetSymbol(LLsymb));
			break;
		case LL_DELETE:
			error(NULL, "Unexpected %s", LLgetSymbol(LLsymb));
			break;
		default:
			error(NULL, "Expected %s before %s", LLgetSymbol(symb), LLgetSymbol(LLsymb));
			break;
	}
	switch (LLsymb) {
		case IDENTIFIER:
		case NUMBER:
		case LITERAL:
		case STRING:
			continueMessage(" %s", yytext);
			break;
		default:
			break;
	}
	endMessage();
}

} /* end of C code */
%token IDENTIFIER, NUMBER, LITERAL, STRING;

%label MISSING_ELEMENT, "literal, identifier or term";
%label MISSING_LITERAL_OR_IDENTIFIER, "literal or identifier";
%label MISSING_STRING, "string";
%label MISSING_IDENTIFIER, "identifier";

%label TOKEN, "%token";
%label START, "%start";
%label PERSISTENT, "%persistent";
%label IF, "%if";
%label WHILE, "%while";
%label AVOID, "%avoid";
%label PREFER, "%prefer";
%label DEFAULT, "%default";
%label LEXICAL, "%lexical";
%label PREFIX, "%prefix";
%label ONERROR, "%onerror";
%label FIRST, "%first";
%label C_DECL, "C code";
%label C_EXPR, "C expression";
%label LABEL, "%label";
%label INCLUDE, "%include";
%label OPTIONS, "%options";
%label DATATYPE, "%datatype";
%label TOP, "%top";

%label BACKREF, "...";
%label DOTQMARK, "..?";

%start parser, specification;

specification :
	[ declaration ]*
;

identifier<Token *> :
	IDENTIFIER
	{
		LLretval = newToken();
	}
|
	%default
	MISSING_IDENTIFIER
;

declaration
{
	identifier = NULL;
	secondIdentifier = NULL;
} :
	C_DECL
	{
		CCode *code = newCCode();
		newDeclaration(CODE, code);
	}
|
	START
	identifier
	','
	identifier<secondIdentifier>
	{
		if (identifier != NULL && secondIdentifier != NULL) {
			Directive *directive = newDirective(START_DIRECTIVE, identifier, secondIdentifier);
			newDeclaration(DIRECTIVE, directive);
		} else {
			freeToken(identifier);
			freeToken(secondIdentifier);
		}
	}
	';'
|
	TOKEN
	identifier
	{
		if (identifier != NULL) {
			Directive *directive = newDirective(TOKEN_DIRECTIVE, identifier, NULL);
			newDeclaration(DIRECTIVE, directive);
			identifier = NULL;
		}
	}
	[
		','
		identifier
		{
			if (identifier != NULL) {
				Directive *directive = newDirective(TOKEN_DIRECTIVE, identifier, NULL);
				newDeclaration(DIRECTIVE, directive);
				identifier = NULL;
			}
		}
	]*
	';'
|
	FIRST
	identifier
	','
	identifier<secondIdentifier>
	{
		if (identifier != NULL && secondIdentifier != NULL) {
			Directive *directive = newDirective(FIRST_DIRECTIVE, identifier, secondIdentifier);
			newDeclaration(DIRECTIVE, directive);
		} else {
			freeToken(identifier);
			freeToken(secondIdentifier);
		}
	}
	';'
|
	LEXICAL
	identifier
	{
		if (identifier != NULL) {
			Directive *directive = newDirective(LEXICAL_DIRECTIVE, identifier, NULL);
			newDeclaration(DIRECTIVE, directive);
		}
	}
	';'
|
	PREFIX
	identifier
	{
		if (identifier != NULL) {
			Directive *directive = newDirective(PREFIX_DIRECTIVE, identifier, NULL);
			newDeclaration(DIRECTIVE, directive);
		}
	}
	';'
|
	ONERROR
	identifier
	{
		if (identifier != NULL) {
			Directive *directive = newDirective(ONERROR_DIRECTIVE, identifier, NULL);
			newDeclaration(DIRECTIVE, directive);
		}
		error(NULL, "%%onerror not supported (yet)\n");
	}
	';'
|
	LABEL
	[
		IDENTIFIER
		{
			if (!option.llgenMode)
				identifier = newToken();
		}
	|
		LITERAL
		{
			if (!option.llgenMode)
				identifier = newToken();
		}
	|
		%default
		MISSING_LITERAL_OR_IDENTIFIER
	]
	','
	[
		STRING
		{
			if (!option.llgenMode)
				secondIdentifier = newToken();
		}
	|
		%default
		MISSING_STRING
	]
	{
		if (option.llgenMode) {
			error(NULL, "%%label is not allowed in LLgen mode\n");
			freeToken(identifier);
			freeToken(secondIdentifier);
		} else if (identifier != NULL && secondIdentifier != NULL) {
			Directive *directive = newDirective(LABEL_DIRECTIVE, identifier, secondIdentifier);
			newDeclaration(DIRECTIVE, directive);
		} else {
			freeToken(identifier);
			freeToken(secondIdentifier);
		}
	}
	';'
|
	INCLUDE
	[
		STRING
		{
			if (!(option.llgenMode || option.LLgenStyleOutputs)) {
				identifier = newToken();
			}
		}
	|
		%default
		MISSING_STRING /* identifier is NULL by default */
	]
	';'
	{
		if (option.llgenMode) {
			error(NULL, "%%include is not allowed in LLgen mode\n");
		} else if (option.LLgenStyleOutputs) {
			error(NULL, "%%include is not allowed with LLgen-style outputs\n");
		} else if (identifier != NULL) {
			if (openInclude(identifier))
				parser();
			freeToken(identifier);
		}
	}
|
	OPTIONS
	[
		STRING
		{
			if (!option.llgenMode)
				identifier = newToken();
		}
	|
		%default
		MISSING_STRING /* token is NULL by default */
	]
	';'
	{

		/* Most options will override the command line, except for:
			--base-name
			--error-limit
			--verbose
			--token-pattern
		*/
		if (option.llgenMode) {
			error(NULL, "%%options is not allowed in LLgen mode\n");
		} else if (identifier != NULL) {
			/* NOTE: this name is fixed, as all the macro's below use it! */
			char *optstring;

			char *strtokState;
			char *processedString = processString(identifier);

			if (processedString == NULL) {
				warning(WARNING_UNMASKED, identifier, "Error parsing %%options string\n");
			} else if ((optstring = strtokReentrant(processedString, " \t\n", &strtokState)) != NULL) {
				bool nonSupported = false;
				do {
					PREPROCESS_OPTION()

					LOCAL_OPTION("verbose", OPTIONAL_ARG)
						if (!option.verboseSet) {
							if (optarg != NULL) {
								PARSE_INT(option.verbose, 1, MAX_VERBOSITY);
							} else {
								option.verbose++;
								/* Prevent overflow */
								if (option.verbose > MAX_VERBOSITY)
									option.verbose = MAX_VERBOSITY;
							}
						} else {
							warning(WARNING_OPTION_OVERRIDE, identifier, "Will not override verbose option passed on command line\n");
						}
					LOCAL_END_OPTION
					LOCAL_OPTION("suppress-warnings", OPTIONAL_ARG)
						checkWarningSuppressionArguments(optarg, (int) optlength, optstring);
					LOCAL_END_OPTION

					LOCAL_OPTION("max-compatibility", NO_ARG)
						option.LLgenArgStyle = true;
						option.onlyLLgenEscapes = true;
						option.LLgenStyleOutputs = true;
					LOCAL_END_OPTION
					LOCAL_BOOLEAN_OPTION("warnings-as-errors", option.warningsErrors)
					LOCAL_OPTION("base-name", REQUIRED_ARG)
						if (!option.outputBaseNameSet) {
							option.outputBaseName = safeStrdup(optarg, "parseCmdLine");
							option.outputBaseNameSet = true;
							option.outputBaseNameLocation = newPlaceHolder();
						} else {
							if (option.outputBaseNameLocation == NULL) {
								warning(WARNING_OPTION_OVERRIDE, identifier, "Will not override base-name option passed on command line\n");
							} else {
								warning(WARNING_UNMASKED, identifier, "Option base-name already set at ");
								printAt(option.outputBaseNameLocation);
								continueMessage("\n");
							}
						}
					LOCAL_END_OPTION
					LOCAL_OPTION("extensions", REQUIRED_ARG)
						if (option.extensions == NULL) {
							option.extensions = split(optarg, ",", true);
							option.extensionsLocation = newPlaceHolder();
						} else {
							if (option.extensionsLocation == NULL) {
								warning(WARNING_OPTION_OVERRIDE, identifier, "Will not override extensions option passed on command line\n");
							} else {
								warning(WARNING_UNMASKED, identifier, "Option extensions already set at ");
								printAt(option.extensionsLocation);
								continueMessage("\n");
							}
						}
					LOCAL_END_OPTION
					LOCAL_BOOLEAN_OPTION("llgen-arg-style", option.LLgenArgStyle)
					LOCAL_BOOLEAN_OPTION("llgen-escapes-only", option.onlyLLgenEscapes)
					LOCAL_BOOLEAN_OPTION("no-prototypes-header", option.noPrototypesHeader)
					LOCAL_BOOLEAN_OPTION("no-line-directives", option.dontGenerateLineDirectives)
					LOCAL_BOOLEAN_OPTION("llgen-output-style", option.LLgenStyleOutputs)
					LOCAL_BOOLEAN_OPTION("reentrant", option.reentrant)
					LOCAL_BOOLEAN_OPTION("no-eof-zero", option.noEOFZero)
					LOCAL_BOOLEAN_OPTION("no-init-llretval", option.noInitLLretval)
					LOCAL_BOOLEAN_OPTION("no-llreissue", option.noLLreissue)
					LOCAL_OPTION("generate-lexer-wrapper", OPTIONAL_ARG)
						if (option.generateLexerWrapperSpecified) {
							if (option.generateLexerWrapperLocation == NULL) {
								warning(WARNING_UNMASKED, identifier, "Option generate-lexer-wrapper also set on command line\n");
							} else {
								warning(WARNING_UNMASKED, identifier, "Option generate-lexer-wrapper also set at ");
								printAt(option.generateLexerWrapperLocation);
								continueMessage("\n");
							}
						}
						option.generateLexerWrapperSpecified = true;
						option.generateLexerWrapperLocation = newPlaceHolder();
						if (optarg == NULL) {
							option.generateLexerWrapper = true;
						} else {
							if (strcmp(optarg, "no") == 0)
								option.generateLexerWrapper = false;
							else if (strcmp(optarg, "yes") == 0)
								option.generateLexerWrapper = true;
							else
								fatal("Argument '%s' is invalid for option generate-lexer-wrapper\n", optarg);
						}
					LOCAL_END_OPTION
					LOCAL_BOOLEAN_OPTION("generate-llmessage", option.generateLLmessage)
					LOCAL_BOOLEAN_OPTION("generate-symbol-table", option.generateSymbolTable)
					LOCAL_BOOLEAN_OPTION("abort", option.abort)
					LOCAL_BOOLEAN_OPTION("no-allow-label-create", option.noAllowLabelCreate)
					LOCAL_OPTION("error-limit", REQUIRED_ARG)
						if (!option.errorLimitSet)
							PARSE_INT(option.errorLimit, 0, INT_MAX);
						else
							warning(WARNING_OPTION_OVERRIDE, identifier, "Will not override error-limit option passed on the command line\n");
					LOCAL_END_OPTION
					LOCAL_BOOLEAN_OPTION("no-arg-count", option.noArgCount);
					LOCAL_BOOLEAN_OPTION("lowercase-symbols", option.lowercaseSymbols);
					LOCAL_BOOLEAN_OPTION("not-only-reachable", option.notOnlyReachable);
					LOCAL_BOOLEAN_OPTION("thread-safe", option.threadSafe)
#ifdef USE_REGEX
					LOCAL_OPTION("token-pattern", REQUIRED_ARG)
						int result;
						if (!option.useTokenPattern) {
							option.useTokenPattern = true;
							if ((result = regcomp(&option.tokenPattern, optarg, REG_EXTENDED | REG_NOSUB)) != 0) {
								char errorBuffer[100];
								regerror(result, &option.tokenPattern, errorBuffer, 100);
								fatal("Regular expression could not be compiled: %s\n", errorBuffer);
							}
						} else {
							warning(WARNING_OPTION_OVERRIDE, identifier, "Will not override token-pattern specified on the command line\n");
						}
					LOCAL_END_OPTION
#else
					LOCAL_OPTION("token-pattern", OPTIONAL_ARG)
						fatal("This LLnextgen binary does not support the '%.*s' option. See the documentation for details\n", (int) optlength, optstring);
					LOCAL_END_OPTION
#endif
					LOCAL_OPTION("gettext", OPTIONAL_ARG)
						checkGettextArguments(optarg);
					LOCAL_END_OPTION

					/* Show appropriate message for options only supported on command line */
					LOCAL_OPTION_USED("depend", nonSupported)
					LOCAL_OPTION_USED("depend-cpp", nonSupported)
					LOCAL_OPTION_USED("dump-lexer-wrapper", nonSupported)
					LOCAL_OPTION_USED("dump-llmessage", nonSupported)
					LOCAL_OPTION_USED("dump-tokens", nonSupported)
					LOCAL_OPTION_USED("help", nonSupported)
					LOCAL_OPTION_USED("keep-dir", nonSupported)
					LOCAL_OPTION_USED("show-dir", nonSupported)
					LOCAL_OPTION_USED("version", nonSupported)

					if (nonSupported)
						fatal("Option '%.*s' is not supported in %%options\n", (int) optlength, optstring);

					error(identifier, "Option '%.*s' does not exist\n", (int) optlength, optstring);
				} while ((optstring = strtokReentrant(NULL, " \t\n", &strtokState)) != NULL);

				/* After this point, the string will not be used anymore. */
				free(processedString);

				postOptionChecks();
			}
			/* After this point, the identifier will not be used anymore. */
			freeToken(identifier);
		}
	}
|
	DATATYPE
	[
		STRING
		{
			if (!option.llgenMode)
				identifier = newToken();
		}
	|
		%default
		MISSING_STRING
	]
	[
		','
		[
			STRING
			{
				if (!option.llgenMode)
					secondIdentifier = newToken();
			}
		|
			%default
			MISSING_STRING
		]
	]?
	';'
	{
		if (option.llgenMode) {
			error(NULL, "%%datatype is not allowed in LLgen mode\n");
		} else {
			Directive *directive = newDirective(DATATYPE_DIRECTIVE, identifier, secondIdentifier);
			newDeclaration(DIRECTIVE, directive);
		}
	}
|
	TOP
	C_DECL
	{
		if (top_code != NULL)
			error(NULL, "Only one section of C code may be marked with %%top\n");
		else if (option.llgenMode)
			error(NULL, "%%top is not supported in LLgen mode\n");
		else
			top_code = newCCode();
	}
|
	rule ';'
;

rule
{
	NonTerminal *nonTerminal;
	CCode *expr = NULL, *decl = NULL;
	List *rule;
	int argCount = 0;

	retvalIdent = NULL;
} :
	identifier<name> /* Name */
	[
		'<'
		identifier<retvalIdent>
		[
			[
				'*'
			|
				IDENTIFIER
			]
			{
				if (retvalIdent != NULL)
					safeStrcatWithSpace(&retvalIdent->text, yytext, "grammar:rule");
			}
		]*
		'>'
	]?
	[
		C_EXPR
		{
			expr = newCCode();
			/* The input may not be completely valid, but we can still do
			   checking without worying about seg-faults and the like. We may
			   generate an extra error message, but that's OK. */
			if (!option.noArgCount)
				argCount = determineArgumentCount(expr, true);
		}
	]?		/* Parameters */
	[
		C_DECL
		{
			decl = newCCode();
		}
	]?		/* Local variable definitions */
	':'
	{
		rule = newList();
		nonTerminal = newNonTerminal(name, retvalIdent, expr, decl, rule, argCount);
		if (name != NULL)
			newDeclaration(NONTERMINAL, nonTerminal);
	}
	productions(&nonTerminal->term)
;

productions(Term *term)
{
	Alternative *alternative;
} :
	{
		alternative = newAlternative();
		alternative->enclosing = term;
		listAppend(term->rule, alternative);
	}
	simpleproduction(alternative)
	[
		'|'
		{
			alternative = newAlternative();
			alternative->enclosing = term;
			listAppend(term->rule, alternative);
		}
		simpleproduction(alternative)
	]*
;

simpleproduction(Alternative *alternative) :
	[
		DEFAULT
		{
			if (alternative->flags & ALTERNATIVE_DEFAULT)
				error(NULL, "%%default specified more than once on a single alternative\n");
			alternative->flags |= ALTERNATIVE_DEFAULT;
		}
	|
		IF
		{
			if (alternative->flags & ALTERNATIVE_CONDITION)
				error(NULL, "More than one condition specified on a single alternative\n");
			alternative->flags |= ALTERNATIVE_IF;
		}
		C_EXPR
		{
			alternative->expression = newCCode();
		}
	|
		PREFER
		{
			if (alternative->flags & ALTERNATIVE_CONDITION)
				error(NULL, "More than one condition specified on a single alternative\n");
			alternative->flags |= ALTERNATIVE_PREFER;
		}
	|
		AVOID
		{
			if (alternative->flags & ALTERNATIVE_CONDITION)
				error(NULL, "More than one condition specified on a single alternative\n");
			alternative->flags |= ALTERNATIVE_AVOID;
		}
	]*
	[
		%persistent
		element<grammarPart>
		repeats
		{
			switch (grammarPart->subtype) {
				case PART_ACTION:
					if (repeats.number != 1 || repeats.subtype != FIXED)
						error(grammarPart->uAction, "Repetition specified on action\n");
					break;
				case PART_UNDETERMINED:
				case PART_LITERAL:
					if (repeats.subtype & FIXED && repeats.number == 1)
						break;

					/* This will make it a Term, so treat it like one! */
					grammarPart = wrapInTerm(grammarPart);
					/* FALLTHROUGH */
				case PART_TERM:
					/* I'm not merging in terms with a single alternative, because I want to
					   keep the information about conflict resolvers. Then we can bug the user ;-) */
					grammarPart->uTerm.repeats = repeats;
					grammarPart->uTerm.index = listSize(alternative->parts);
					grammarPart->uTerm.enclosing.alternative = alternative;
					break;
				case PART_UNKNOWN:
					break;
				case PART_BACKREF:
					if (repeats.number != 1 || repeats.subtype != FIXED)
						softError(grammarPart->token, "Repetition specification on back-reference operator\n");
					break;
				default:
					PANIC();
			}
			listAppend(alternative->parts, grammarPart);
		}
	]*
;

element<GrammarPart *> :
	%default
	MISSING_ELEMENT
	{
		LLretval = newGrammarPart(NULL, PART_UNKNOWN);
	}
|
	C_DECL	/* Action */
	{
		LLretval = newGrammarPart(NULL, PART_ACTION);
		LLretval->uAction = newCCode();
	}
|
	'['
	{
		LLretval = newGrammarPart(newToken(), PART_TERM);
	}
	[
		WHILE
		C_EXPR
		{
			if (LLretval->uTerm.flags & TERM_WHILE) {
				error(NULL, "%%while specified more than once in a single term\n");
			} else {
				LLretval->uTerm.flags |= TERM_WHILE;
				LLretval->uTerm.expression = newCCode();
			}
		}
	|
		PERSISTENT
		{
			if (LLretval->uTerm.flags & TERM_PERSISTENT)
				error(NULL, "%%persistent specified more than once in a single term\n");
			LLretval->uTerm.flags |= TERM_PERSISTENT;
		}
	]*
	/* List is already initialised as part of newGrammarPart */
	productions(&LLretval->uTerm)
	']'
|
	LITERAL
	{
		LLretval = newGrammarPart(newToken(), PART_LITERAL);
		/* GRAM_TERMINAL(LLretval) = literalToTerminal((LLretval)->token->text); */
	}
|
	IDENTIFIER /* Both terminal and non-terminal */
	{
		LLretval = newGrammarPart(newToken(), PART_UNDETERMINED);
	}
	[
		'<'
		identifier
		{
			LLretval->uNonTerminal.retvalIdent = identifier;
			if (identifier != NULL) {
				if (strcmp("LLretval", identifier->text) == 0)
					LLretval->uNonTerminal.flags |= CALL_LLRETVAL;
				else if (strcmp("LLdiscard", identifier->text) == 0)
					LLretval->uNonTerminal.flags |= CALL_LLDISCARD;
			}
		}
		'>'
	]?
	[
		C_EXPR
		{
			(LLretval)->uNonTerminal.expression = newCCode();
		}
	]? /* Parameters */
|
	BACKREF
	{
		LLretval = newGrammarPart(newToken(), PART_BACKREF);
		if (option.llgenMode)
			error(LLretval->token, "Back-reference operator is not supported in LLgen mode\n");
	}
;

repeats<Repeats> :
	/* No repetition operator specified, so exactly one match is requested */
	{
		LLretval.subtype = FIXED;
		LLretval.number = 1;
	}
|
	[
		'*'
		{
			LLretval.subtype = STAR;
			LLretval.number = -1;
		}

	|
		'+'
		{
			LLretval.subtype = PLUS;
			LLretval.number = -1;
		}
	]
	[
		NUMBER
		{
			long value;
			errno = 0;
			value = strtol(yytext, NULL, 10);
			if (errno == ERANGE || value > INT_MAX)
				error(NULL, "Number of repetitions too large\n");
			if (value <= 0) {
				PANIC();
				/* The lexer only returns numbers without a sign, therefore the
				   parser should PANIC when it does happen. */
			}
			LLretval.number = value;
			if (LLretval.number == 1) {
				if (LLretval.subtype == PLUS) {
					LLretval.subtype = FIXED;
					warning(WARNING_UNMASKED, NULL, "Fixed single repetition specified as '+1'\n");
				} else if (LLretval.subtype == STAR) {
					warning(WARNING_UNMASKED, NULL, "Optional single repetition specified as '*1' (instead of '?')\n");
				} else {
					PANIC();
					/* If the subtype is not PLUS it should always be STAR. If it is not,
					   something is very wrong. */
				}
			}
		}
	]?
|
	NUMBER
	{
		long value;
		LLretval.subtype = FIXED;
		errno = 0;
		value = strtol(yytext, NULL, 10);
		if (errno == ERANGE || value > 32767)
			error(NULL, "Number of repetitions too large\n");
		if (value <= 0) {
			PANIC();
			/* The lexer only returns numbers without a sign, therefore the
			   parser should PANIC when it does happen. */
		}
		LLretval.number = value;
	}
|
	'?'
	{
		LLretval.subtype = STAR;
		LLretval.number = 1;
	}
|
	DOTQMARK
	{
		LLretval.subtype = FINOPT;
		LLretval.number = 1;
	}
;
