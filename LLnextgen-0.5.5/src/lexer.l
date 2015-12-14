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

%{

#include <string.h>
#include <errno.h>

#include "nonRuleAnalysis.h"
#include "globals.h"
#include "grammar.h"
#include "lexer.h"
#include "option.h"

/* These variables don't need to be saved when processing include files,
   because they are only valid during a call to the lexer, not in between
   calls.
*/
static int braceCount,
			parenCount,
			atomStart,
			caller;
static bool allowDirectiveHere,
			inCode = false,
			inExpression = false;

int lineNumber = 1;
int codeStart; 	/* codeStart should be used for C_EXPR and
				   C_DECL to indicate the first line number */
const char *fileName = "<stdin>";

List *inputStack;

#define CALL(x) caller = YYSTATE; BEGIN(x)
#define RETURN() BEGIN(caller)

/* Replace flex's default fatal error handler by ours. */
#define YY_FATAL_ERROR fatal

typedef struct {
	const char *fileName;
	char *savedText;
	int lineNumber;
	int codeStart;
	YY_BUFFER_STATE	bufferState;
} Input;

%}

%x	expr decl comment string literal directive lineComment
%option never-interactive
%option nounput

LETTER			[a-zA-Z]
LETTER_DIGIT	[a-zA-Z0-9]
ALL_IDENT_CHAR	[a-zA-Z_0-9]

IDENT_PAT		({LETTER}{ALL_IDENT_CHAR}*|_+{LETTER_DIGIT}{ALL_IDENT_CHAR}*)
ILL_RESERVED	%{IDENT_PAT}

%%
\'				{ yymore(); atomStart = lineNumber; CALL(literal); }
\"				{ yymore(); atomStart = lineNumber; CALL(string); }
%token			return TOKEN;
%start			return START;
%persistent		return PERSISTENT;
%if				return IF;
%while			return WHILE;
%avoid			return AVOID;
%prefer			return PREFER;
%default		return DEFAULT;
%lexical		return LEXICAL;
%prefix			return PREFIX;
%onerror		return ONERROR;
%first			return FIRST;
%label			return LABEL;
%include		return INCLUDE;
%options		return OPTIONS;
%datatype		return DATATYPE;
%top			return TOP;
{ILL_RESERVED}	{ error(0, "Illegal reserved word %s\n", yytext); }
\.\.\.			return BACKREF;
\.\.\?			return DOTQMARK;
[0-9]+			return NUMBER;
{IDENT_PAT}		return IDENTIFIER;
\(				{
					braceCount = 0;
					parenCount = 0;
					allowDirectiveHere = false;
					codeStart = lineNumber;
					yymore();
					BEGIN(expr);
					inExpression = true;
				}
\{				{
					braceCount = 0;
					parenCount = 0;
					allowDirectiveHere = false;
					codeStart = lineNumber;
					yymore();
					BEGIN(decl);
					inCode = true;
				}
[ \t\r]			/* Eat up whitespace */
\/\/			{ atomStart = lineNumber; CALL(lineComment); }
\/\*			{ atomStart = lineNumber; CALL(comment); }
\n				lineNumber++;
	/* FIXME: These two rules can be insterted to reduce the number of errors for UTF-8 type characters in the input
		However, you still get lots and lots of messages for e.g. C-code.
[][,;:|*+?]		return * (unsigned char *) yytext;
[^][,;:|*+?%\'\"%0-9a-zA-Z_\(\{ \t\r/\n]+	{ error(0, "Illegal character(s) in input: '%s'\n", yytext); }
	*/
.				return * (unsigned char *) yytext;

	/* Yes, start-condition scopes are nice. But not POSIX conformant :-( */
<comment>\n				{ yymore(); lineNumber++; }
<comment>\*\/			{
							if (caller != INITIAL)
								yymore();
							RETURN();
						}
<comment>.				yymore(); /* Eat up any comment character */

<lineComment>\\\n		{ yymore(); lineNumber++; }
<lineComment>\n			{
							allowDirectiveHere = true;
							if (caller != INITIAL)
								yymore();
							lineNumber++;
							RETURN();
						}
<lineComment>.			yymore();

<expr>\)				{
							if (parenCount-- == 0) {
								BEGIN(INITIAL);
								inExpression = false;
								return C_EXPR;
							}
							yymore();
							allowDirectiveHere = false;
						}
<decl>\}				{
							if (braceCount-- == 0) {
								if (parenCount != 0)
									warning(WARNING_UNBALANCED_C, NULL, "Unbalanced parentheses in C code\n");
								BEGIN(INITIAL);
								inCode = false;
								return C_DECL;
							}
							yymore();
							allowDirectiveHere = false;
						}
<decl>\)				{ yymore(); parenCount--; allowDirectiveHere = false; }
<expr,decl>\/\/			{ yymore(); atomStart = lineNumber; CALL(lineComment); /* Directives are allowed afterwards; is set within lineComment scope */ }
<expr,decl>\/\*			{ yymore(); atomStart = lineNumber; CALL(comment); /* No change in whether directives are allowed */ }
<expr,decl>\"			{ yymore(); atomStart = lineNumber; CALL(string); allowDirectiveHere = false; }
<expr,decl>\'			{ yymore(); atomStart = lineNumber; CALL(literal); allowDirectiveHere = false; }
<expr,decl>\{			{ yymore(); braceCount++; allowDirectiveHere = false; }
<expr,decl>\(			{ yymore(); parenCount++; allowDirectiveHere = false; }
<expr,decl>\n			{ yymore(); lineNumber++; allowDirectiveHere = true; }
<expr,decl>#			{ yymore(); if (allowDirectiveHere) { CALL(directive); } /* FIXME: do we need to give an error here? */ }
<expr,decl>[ \t]		{ yymore(); /* No change in whether directives are allowed */ }
<expr,decl>.			{ yymore(); allowDirectiveHere = false; }

<string>([^"\\\n]|\\[^\n])+		yymore();
<string>\n				{
							error(0, "String missing closing \"\n");
							lineNumber++;
							RETURN();
							if (caller == INITIAL)
								return STRING;
							yymore();
						}
<string>\\\n			{ yymore(); lineNumber++; }
<string>\"				{
							RETURN();
							if (caller == INITIAL)
								return STRING;
							yymore();
						}
<string>\\				yymore();

<literal>([^'\\\n]|\\[^\n])+		yymore();
<literal>\n				{
							error(0, "Literal missing closing \'\n");
							lineNumber++;
							RETURN();
							if (caller == INITIAL)
								return LITERAL;
							yymore();
						}
<literal>\'				{
							RETURN();
							if (caller == INITIAL)
								return LITERAL;
							yymore();
						}
<literal>\\				yymore();

<directive>\\\n			{ yymore(); lineNumber++; }
<directive>\n			{ yymore(); lineNumber++; RETURN(); }
<directive>.			yymore();

	/* <<EOF>> again isn't POSIX, but this solves the NUL-character problem */
<*><<EOF>>				{ return EOFILE; }
%%

static bool popInclude;
List *includedFiles;

/** Open a file indicated by a %include directive.
	@param file The token representing the file to be included.
	@return A boolean indicating whether opening the file was successfull.

	This function tries to open an include file, but also prevents recursive
	inclusion of files. If a file cannot be opened, it will issue an error
	message, but further processing will continue.
*/
bool openInclude(Token *file) {
	Input *input;
	char *tmpFileName;
	FILE *inputFile;
	int i;

	if ((tmpFileName = processString(file)) == NULL) {
		error(file, "Error processing %%include string\n");
		return false;
	}

	/* Try to prevent recursive includes.
	   First check if it is trying to include itself... */
	if (strcmp(fileName, tmpFileName) == 0) {
		error(file, "Recursive %%include detected. Refusing to include file.\n");
		free(tmpFileName);
		return false;
	}
	/* then look for already included files, for the more complicated cases. */
	for (i = 0; i < listSize(inputStack); i++) {
		input = (Input *) listIndex(inputStack, i);
		if (strcmp(input->fileName, tmpFileName) == 0) {
			error(file, "Recursive %%include detected. Refusing to include file.\n");
			free(tmpFileName);
			return false;
		}
	}

	if ((inputFile = fopen(tmpFileName, "r")) == NULL) {
		error(file, "Can't include file %s: %s\n", tmpFileName, strerror(errno));
		free(tmpFileName);
		return false;
	}

	/* Save current state. */
	input = (Input *) safeMalloc(sizeof(Input), "openInclude");
	input->fileName = fileName;
	input->lineNumber = lineNumber;
	input->codeStart = codeStart;
	input->bufferState = YY_CURRENT_BUFFER;
#ifndef LL_VERSION
	if (LLreissue)
#else
	if (LLreissue != LL_NEW_TOKEN)
#endif
		input->savedText = safeStrdup(yytext, "openInclude");
	else
		input->savedText = NULL;
	listAppend(inputStack, input);

	/* Set up new state. */
	fileName = tmpFileName;
	lineNumber = 1;

/* If we are in debug mode, we always save the names of all included files,
   so that we can free them later. Normal operation only requires this list
   when generating dependency information. */
#ifndef MEM_DEBUG
	if (option.depend)
#endif
		listAppend(includedFiles, tmpFileName);

	yy_switch_to_buffer(yy_create_buffer(inputFile, YY_BUF_SIZE));
	return true;
}

/** Function to switch back from an include file.

	This function is called by the lexer when the end of a file has been
	reached. Error messages for unterminated ... are generated here. The lexer
	is never directed to continue scanning, because we are using a reentrant
	parser which will end the current parser and go back to the outer parser.
	The outer parser will then call yylex again to continue scanning.
*/
int yywrap(void) {
	fclose(yyin);

	/* Check for unterminated sub lexers. */
	if (YYSTATE != INITIAL) {
		Token token;
		token.fileName = fileName;
		switch(YYSTATE) {
			case expr:
				token.lineNumber = codeStart;
				error(&token, "Unterminated expression\n");
				inExpression = false;
				break;
			case decl:
				token.lineNumber = codeStart;
				error(&token, "Unterminated code\n");
				inCode = false;
				break;
			case comment:
				token.lineNumber = atomStart;
				error(&token, "Unterminated comment\n");
				break;
			case lineComment:
				if (inExpression) {
					token.lineNumber = codeStart;
					error(&token, "Unterminated expression\n");
					inExpression = false;
				} else if (inCode) {
					token.lineNumber = codeStart;
					error(&token, "Unterminated code\n");
					inCode = false;
				}
				break;
			case string:
				token.lineNumber = atomStart;
				error(&token, "Unterminated string\n");
				break;
			case literal:
				token.lineNumber = atomStart;
				error(&token, "Unterminated character literal\n");
				break;
			case directive:
				token.lineNumber = codeStart;
				error(&token, "Unterminated %s\n", caller == expr ? "expression" : "code");
				break;
			default:
				PANIC();
		}
		BEGIN(INITIAL);
	}

	/* Check whether input stack is empty. Files on the input stack were
	   pushed by a %include directive. */
	if (listSize(inputStack) > 0)
		popInclude = true;

	return 1;
}

/** Routine called by @a openFirstInput and @a nextFile to start reading from
		a new input.
	@param newInput The new file to read from.

	The reason this function exists is because we cannot do this in an
	external file. This has to be done in the lexer.
*/
static void initLexer(FILE *newInput) {
	BEGIN(INITIAL);
	popInclude = false;
	yyrestart(newInput);
}

/** Open the first input file.

	The first input file can also be standard in. Therefore, the first input
	file requires special handling.
*/
void openFirstInput(void) {
	int size;

	/* Initialise the lexer */
	inputStack = newList();
/* If we are in debug mode, we always save the names of all included files,
   so that we can free them later. Normal operation only requires this list
   when generating dependency information. */
#ifndef MEM_DEBUG
	if (option.depend)
#endif
		includedFiles = newList();
	size = listSize(option.inputFileList);

	if (size == 0) {
		/* We don't have to set fileName, as it defaults to <stdin> */
		initLexer(stdin);
	} else {
		fileName = (char *) listIndex(option.inputFileList, 0);
		initLexer(safeFopen(fileName, "r"));
	}

	/* Initialise the main data-structure */
	declarations = newList();
}

/** Open the next file of the list the user specified.
	@return A boolean to indicate whether there was a next file to open.

	This function does not return if an input file could not be opened.
*/
bool nextFile(void) {
	static int idx = 1;
	int size;

	size = listSize(option.inputFileList);

	if (size != 0 && idx < size) {
		lineNumber = 1;
		fileName = (char *) listIndex(option.inputFileList, idx++);
		initLexer(safeFopen(fileName, "r"));
		return true;
	}
	return false;
}

/** Wrap the lexer so that it abides by the rules LLnextgen lays down for
		lexical analysers.

	This is also the place where the included files are popped of the stack.
	Doing this in yywrap will cause messages about inserting tokens to be
	reported with respect to the wrong file.
*/
int lexerWrapper(void) {
#ifndef LL_VERSION
	static int lastToken;
#endif
	static bool freeText = false;

	if (freeText) {
		free(yytext);
		freeText = false;
	}

	if (popInclude) {
		Input *top;

		/* Free the current buffer */
		yy_delete_buffer(YY_CURRENT_BUFFER);

		popInclude = false;
		top = (Input *) listPop(inputStack);
		fileName = top->fileName;
		lineNumber = top->lineNumber;
		codeStart = top->codeStart;
		yy_switch_to_buffer(top->bufferState);
		if (top->savedText != NULL) {
			yytext = top->savedText;
			freeText = true;
		}
		free(top);
	}

#ifndef LL_VERSION
	if (LLreissue) {
		LLreissue = 0;
		return lastToken;
	} else {
		return lastToken = yylex();
	}
#else
	if (LLreissue == LL_NEW_TOKEN) {
		return yylex();
	} else {
		int retval = LLreissue;
		LLreissue = LL_NEW_TOKEN;
		return retval;
	}
#endif
}

#ifdef MEM_DEBUG
void cleanUpLexer(void) {
	char *name;
	while ((name = listPop(includedFiles)) != NULL)
		free(name);
	deleteList(includedFiles);
	yy_delete_buffer(YY_CURRENT_BUFFER);

/* Unfortunately, Flex doesn't define a preprocessor symbol for the patch level.
   However, the yylex_destroy function was added somewhere between 2.5.4a and
   2.5.33 (don't know where exactly). Therefore the Makefile defines the symbol
   NEWFLEX if the patch level is greater than 4. */
#if YY_FLEX_MAJOR_VERSION > 2 || (YY_FLEX_MAJOR_VERSION == 2 && YY_FLEX_MINOR_VERSION > 5) || NEWFLEX
	yylex_destroy();
#endif
}
#endif
