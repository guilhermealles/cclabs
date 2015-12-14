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

/** @file
	argcount.[ch] contains all the routines used for counting the number of
	arguments declared or passed to a rule. It is assumed the input is more or
	less valid, because it has already been accepted by the lexer.
	
	All functions use the static variables to communicate, so the code is not
	reentrant.
*/

/* FIXME: there is some inconsistency in the parameter parsing. This should be
	removed. Some things are static globals, while others are passed. */

#include <string.h>
#include <ctype.h>

#include "globals.h"
#include "bool.h"
#include "nonRuleAnalysis.h"
#include "option.h"

static int argCount, parenCount;
static size_t length, i;
static bool hasIdent;
static CCode errorToken, *expression;
static char *lastSemiColon = NULL;

/** Skip to the end of a string or character literal.
	@param end The character that will end the string or character literal.
*/
static void skipAtom(char end) {
	for (i++; i < length; i++) {
		if (expression->text[i] == '\n')
			errorToken.lineNumber++;
		/* Make sure we skip embedded ending characters */
		if (expression->text[i] == '\\') {
			i++;
			/* Keep line numbers for error messages correct */
			if (expression->text[i] == '\n')
				errorToken.lineNumber++;	
		}
		else if (expression->text[i] == end) {
			return;
		}
	}
}

/** Skip comments (C and C++ style) embedded in the arguments. */
static void skipComment(void) {
	if (expression->text[i + 1] == '/') {
		/* C++ style comments */
		for (i += 2; i < length; i++) {
			if (expression->text[i] == '\n') {
				errorToken.lineNumber++;
				/* If the end of the line is not preceeded by a backslash,
				   we have found the end of the comment. */
				if (expression->text[i-1] != '\\')
					break;
			}
		}
	} else if (expression->text[i + 1] == '*') {
		/* C style comments */
		for (i += 2; i < length; i++) {
			if (expression->text[i] == '*' && expression->text[i + 1] == '/')
				break;
			if (expression->text[i] == '\n')
				errorToken.lineNumber++;
		}
	}
}

/** Routine to test the validity of an argument separator.
	@param separator The separator character that has to be checked.
	@param header A boolean indicating wheather we are parsing a header.
*/
static void argumentSeparator(char separator, bool header) {
	/* Semi-colons are illegal characters in arguments to a function. */
	if (separator == ';' && !header) {
		softError(&errorToken, "Found ';' in argument\n");
		if (hasIdent)
			argCount++;
		/* No need to convert the semicolon into a comma as we won't be
		   generating code anyway. */
		hasIdent = false;
		return;
	}
	if (parenCount == 0) {
		if (!hasIdent) {
			softError(&errorToken, "No argument found before '%c'\n", separator);
		} else {
			if (separator == ';' && !option.LLgenArgStyle)
				warning(WARNING_ARG_SEPARATOR, &errorToken, "Separating parameters with a ';' is deprecated\n");
			else if (separator == ',' && option.LLgenArgStyle && header)
				warning(WARNING_ARG_SEPARATOR, &errorToken, "Parameters should not be separated with a ','\n");
			argCount++;
		}
		
		/* In the LLgen style of argument definitions, the arguments may end
		   with a semi-colon. Therefore we need to change all but the last 
		   semi-colon into a comma. The last semi-colon is changed to a space,
		   if there is no argument following it.
		
		   Note that having two consequtive semi-colons will already produce
		   an error above, so we don't have to care about generating two
		   consequtive commas in the output.
		*/
		if (lastSemiColon)
			*lastSemiColon = ',';
		
		if (separator == ';')
			lastSemiColon = expression->text + i;
		else
			lastSemiColon = NULL;
		
		hasIdent = false;
	}
}

/** Skip C-preprocessor directives.
	The expression lexer has checked that it is preceded only by white space.
*/
static void skipDirective(void) {
	for (i += 1; i < length; i++) {
		if (expression->text[i] == '\n') {
			errorToken.lineNumber++;
			/* If the end of the line is not preceeded by a backslash,
			   we have found the end of the directive. */
			if (expression->text[i-1] != '\\')
				break;
		}
	}
}

/** @brief Find the number of arguments in an expression.
	@param expression The text containing the expression.
	@param header Boolean indicating whether the expression is part of rule
		header.

	Note: the input to this is already more or less verified by the lexer.
*/
int determineArgumentCount(CCode *code, bool header) {
	/* Semicolons form a problem because they may follow the last argument.
	   Otherwise we could have just overwritten all of them with comma's.
	   Now we have to make sure that a trailing semicolon is overwritten with
	   a space */
	lastSemiColon = NULL;
	
	if (code == NULL)
		return 0;
	
	expression = code;
	errorToken = *expression;
	
	length = strlen(expression->text);
	argCount = parenCount = 0;
	hasIdent = false;
	for (i = 1; i < length; i++) {
		switch (expression->text[i]) {
			case '(': /* For embedded function calls and function pointers */
				parenCount++;
				break;
			case ')':
				parenCount--;
				break;
			case '\n':
				errorToken.lineNumber++;
				break;
			case ',': /* Comma, and for LLgen also semi-colon, separates arguments */
			case ';':
				argumentSeparator(expression->text[i], header);
				break;
			case '/':
				skipComment();
				break;
			case '#':
				skipDirective();
				break;
			case '\\':
				/* Who cares what the user tried to escape here. Just go on.
				   This should not occur anyway, as the lexer has already
				   checked the input. */
				break;
			case '\'':
				if (header) 
					softError(&errorToken, "Character literal found in header\n");
				/* When checking a header, set hadIdent anyway to suppress
				   error messages about missing arguments */
				hasIdent = true;
				skipAtom('\'');
				break;
			case '"':
				if (header) 
					softError(&errorToken, "String literal found in header\n");
				/* When checking a header, set hadIdent anyway to suppress
				   error messages about missing arguments */
				hasIdent = true;
				skipAtom('"');
				break;
			default:
				/* FIXME: prepare checking for unicode characters */
				if (hasIdent)
					break;
				if (isalpha(expression->text[i]) || expression->text[i] == '_')
					hasIdent = true;
				else if (!header && isdigit(expression->text[i]))
					hasIdent = true;
				break;
		}
	}
	
	/* In the LLgen style of argument definitions, the arguments may end
	   with a semi-colon. Therefore we need to change all but the last 
	   semi-colon into a comma. The last semi-colon is changed to a space,
	   if there is no argument following it. */
	if (lastSemiColon) {
		if (hasIdent) {
			*lastSemiColon = ',';
			argCount++;
		} else
			*lastSemiColon = ' ';
	} else if (!hasIdent && argCount != 0) {
		/* If the argCount equals 0, then there are simply no arguments.
		   Otherwise, there has been an argument followed by an argument
		   separator. If there had not been an argument separator, hasIdent
		   would have been set. */
		softError(&errorToken, "No argument found before ')'\n");
	} else if (hasIdent) {
		argCount++;
	}
	
	return argCount;
}

#ifdef ARGCOUNT_TEST
int main(int argc, char *argv[]) {
	CCode code;
	int count;

	if (argc != 2) {
		printf("Usage: test <string>\n");
		return 0;
	}
	code.text = argv[1];
	code.lineNumber = 10;
	code.fileName = "stdin";
	count = determineArgumentCount(&code, false);
	
	printf("Arguments (%d): %s\n", count, code.text);
	return 0;
}
#endif
