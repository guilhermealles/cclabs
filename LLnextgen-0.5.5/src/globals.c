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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "globals.h"
#include "option.h"
#include "os.h"
#include "lexer.h"

/* Variables */
bool errorSeen = false;
bool softErrorSeen = false;

/* Functions */

/** Allocate a block of memory.
	@param size The size of the requested block.
	@param where A string describing the location where the allocation request
		was issued.
	@return A pointer to the newly allocated block of memory.

	The function will not return if the memory could not be allocated.
*/
void *safeMalloc(size_t size, const char *where) {
	void *retval;

	retval = malloc(size);
	if (retval == NULL)
		fatal("Could not allocate memory in %s: %s\n", where, strerror(errno));

	return retval;
}

/** Allocate a cleared block of memory.
	@param size The size of the requested block.
	@param where A string describing the location where the allocation request
		was issued.
	@return A pointer to the newly allocated block of memory.

	The function will not return if the memory could not be allocated. The
	memory is cleared before returning it to the caller.
*/
void *safeCalloc(size_t size, const char *where) {
	void *retval;

	retval = malloc(size);
	if (retval == NULL)
		fatal("Could not allocate memory in %s: %s\n", where, strerror(errno));
	memset(retval, 0, size);
	return retval;
}

/** Resize a previously allocated block of memory.
	@param ptr Pointer to the previously allocated block of memory.
	@param size The requested new size of the block.
	@param where A string describing the location where the reallocation request
		was issued.
	@param A pointer to the reallocated block of memory. As the reallocation
		may cause the block to be moved, the old pointer @a ptr should not be
		used after calling this function.

	The function will not return if the memory could not be allocated.
*/
void *safeRealloc(void *ptr, size_t size, const char *where) {
	ptr = realloc(ptr, size);
	if (ptr == NULL)
		fatal("Could not reallocate memory in %s: %s\n", where, strerror(errno));
	return ptr;
}

/** Allocate and copy a string.
	@param orig The string to copy.
	@param where A string describing the location where the copy request
		was issued.
	@return A pointer to the newly allocated block of memory containing a copy
		of the string.

	The function will not return if the memory could not be allocated.
*/
char *safeStrdup(const char *orig, const char *where) {
	char *copy;
	size_t length;

	/* strdup is non Ansi C :-( */
	length = strlen(orig);
	copy = (char *) malloc(length + 1);
	if (copy == NULL)
		fatal("Could not copy string in %s: %s\n", where, strerror(errno));
	strcpy(copy, orig);
	return copy;
}

/** Allocate and copy a substring.
	@param orig The string to copy.
	@param length The maximum number of characters to copy.
	@param where A string describing the location where the copy request
		was issued.
	@return A pointer to the newly allocated block of memory containing a copy
		of the string.

	The function will not return if the memory could not be allocated.
*/
char *safeStrndup(const char *orig, size_t length, const char *where) {
	char *copy;
	size_t totalLength;

	totalLength = strlen(orig);
	if (length > totalLength)
		length = totalLength;
	copy = (char *) malloc(length + 1);
	if (copy == NULL)
		fatal("Could not copy string in %s: %s\n", where, strerror(errno));
	memcpy(copy, orig, length);
	copy[length] = 0;
	return copy;
}

/** Concatenate two strings separated by an extra space, reallocating storage
	for the first to fit.
	@param base A double pointer to the base string.
	@param append The string to append to @a base.
	@param where A string describing the location where the concatenation request
		was issued.

	The function will not return if the memory could not be allocated.
*/
void safeStrcatWithSpace(char **base, const char *append, const char *where) {
	size_t totalLength, baseLength, appendLength;
	baseLength = strlen(*base);
	appendLength = strlen(append);
	totalLength = baseLength + appendLength + 2;

	if (baseLength > totalLength || appendLength > totalLength)
		fatal("String too long after concatenation in %s\n", where);

	if ((*base = realloc(*base, totalLength)) == NULL)
		fatal("Could not concatenate strings in %s: %s\n", where, strerror(errno));

	*(*base + baseLength) = ' ';
	strcpy(*base + baseLength + 1, append);
}

/** Open a file.
	@param name The name of the file to open.
	@param mode The mode to open the file with. See fopen(3) for details.
	@return A @a FILE pointer to the opened file.

	The function will not return if the file could not be opened with the
	requested mode.
*/
FILE *safeFopen(const char *name, const char *mode) {
	FILE *retval;
	if ((retval = fopen(name, mode)) == NULL) {
		fatal("Could not open '%s': %s\n", name, strerror(errno));
	}
	return retval;
}

/** Alert the user of a fatal error and quit.
	@param fmt The format string for the message. See fprintf(3) for details.
	@param ... The arguments for printing.

	@see fail.
*/
void fatal(const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}

/** Alert the user of a failure and quit.
	@param fmt The format string for the message. See fprintf(3) for details.
	@param ... The arguments for printing.

	The difference from @a fatal is that @a fail is meant to signify bugs in
	the implementation of LLnextgen itself, while @a fatal is meant to signify
	serious problems in the input. @a fail calls @a abort, while @a fatal
	calls @a exit(EXIT_FAILURE)
*/
void fail(const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	abort();
}

static int errorCount = 0;

/** Common part of @a error and @a softError functions.
	@param token The token that was read where the problem occured, or NULL for
		the current location in the file currently being read.
	@param fmt The format string for the message. See fprintf(3) for details.
	@param args The arguments for printing.
*/
static void errorCommon(Token *token, const char *fmt, va_list args) {
	const char *screenFileName;
	int screenLineNumber;

	if (token) {
		screenFileName = token->fileName;
		screenLineNumber = token->lineNumber;
	} else {
		screenFileName = fileName;
		screenLineNumber = lineNumber;
	}

	if (!option.showDir)
		screenFileName = baseName(screenFileName);

	errorCount++;
	fprintf(stderr, "%s:%d: error: ", screenFileName, screenLineNumber);
	vfprintf(stderr, fmt, args);

	if (option.errorLimit > 0 && errorCount >= option.errorLimit) {
		fprintf(stderr, "Error limit reached. Aborting...\n");
		exit(EXIT_FAILURE);
	}
}


/** Alert the user of an error.
	@param token The token that was read where the problem occured, or NULL for
		the current location in the file currently being read.
	@param fmt The format string for the message. See fprintf(3) for details.
	@param ... The arguments for printing.
*/
void error(Token *token, const char *fmt, ...) {
	va_list args;

	errorSeen = true;
	va_start(args, fmt);
	errorCommon(token, fmt, args);
	va_end(args);
}

/** Alert the user of an soft error.
	@param token The token that was read where the problem occured, or NULL for
		the current location in the file currently being read.
	@param fmt The format string for the message. See fprintf(3) for details.
	@param ... The arguments for printing.

	Soft errors differ from normal errors in that they allow analysis of the
	rules to proceed as normal. These are used in the first phase of the program
	when name resolution is performed. Only to be used for errors that do not
	impair the rule analysis.
*/
void softError(Token *token, const char *fmt, ...) {
	va_list args;

	softErrorSeen = true;
	va_start(args, fmt);
	errorCommon(token, fmt, args);
	va_end(args);
}


/** Alert the user of a warning.
	@param warningType The type of warning, for suppression purposes.
	@param token The token that was read where the problem occured, or NULL for
		the current location in the file currently being read.
	@param fmt The format string for the message. See fprintf(3) for details.
	@param ... The arguments for printing.
*/
void warning(int warningType, Token *token, const char *fmt, ...) {
	va_list args;
	const char *screenFileName;
	int screenLineNumber;

	if (option.suppressWarnings)
		return;

	if (warningType & option.suppressWarningsTypes)
		return;

	if (warningType == WARNING_UNUSED && option.unusedIdentifiers != NULL && listSize(option.unusedIdentifiers) > 0) {
		int i;
		for (i = 0; i < listSize(option.unusedIdentifiers); i++) {
			const char *identifier = (const char *) listIndex(option.unusedIdentifiers, i);
			if (strcmp(token->text, identifier) == 0)
				return;
		}
	}

	if (token) {
		screenFileName = token->fileName;
		screenLineNumber = token->lineNumber;
	} else {
		screenFileName = fileName;
		screenLineNumber = lineNumber;
	}

	if (!option.showDir)
		screenFileName = baseName(screenFileName);

	if (option.warningsErrors) {
		errorCount++;
		errorSeen = true;
	}

	fprintf(stderr, "%s:%d: %s: ", screenFileName, screenLineNumber, option.warningsErrors ? "error" : "warning");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if (option.errorLimit > 0 && errorCount >= option.errorLimit) {
		fprintf(stderr, "Error limit reached. Aborting...\n");
		exit(EXIT_FAILURE);
	}
}

/** Alert the user of a general warning.
	@param warningType The type of warning, for suppression purposes.
	@param fmt The format string for the message. See fprintf(3) for details.
	@param ... The arguments for printing.

	The difference between an warning and a generalWarning is that a warning
	refers to a specific point in the input while a general warning can not be
	linked to such a point.
*/
void generalWarning(int warningType, const char *fmt, ...) {
	va_list args;

	if (option.suppressWarnings)
		return;

	if (warningType & option.suppressWarningsTypes)
		return;

	if (option.warningsErrors) {
		errorCount++;
		errorSeen = true;
	}

	fprintf(stderr, "%s: ", option.warningsErrors ? "error" : "warning");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if (option.errorLimit > 0 && errorCount >= option.errorLimit) {
		fprintf(stderr, "Error limit reached. Aborting...\n");
		exit(EXIT_FAILURE);
	}
}


/** Continue a previously started warning or error message.
	@param fmt The format string for the message. See fprintf(3) for details.
	@param ... The arguments for printing.

	The reason for this function is that @a warning and @a error both start
	their message with the location where the error was detected. Sometimes
	though, it is impossible to print the message with one statement.
*/
void continueMessage(const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

/** Print a file location.
	@param token The token that was read where the problem occured, or NULL for
		the current location in the file currently being read.
*/
void printAt(Token *token) {
	const char *screenFileName;
	int screenLineNumber;

	if (token) {
		screenFileName = token->fileName;
		screenLineNumber = token->lineNumber;
	} else {
		screenFileName = fileName;
		screenLineNumber = lineNumber;
	}

	if (!option.showDir)
		screenFileName = baseName(screenFileName);

	fprintf(stderr, "%s:%d", screenFileName, screenLineNumber);
}

/** End a message.

	If the last part of a message was printed by @a printAt, this function can
	be used to end the line it was printed on.
*/
void endMessage(void) {
	fputc('\n', stderr);
}

/** Convert a string from the input format to an internally usable string.
	@param string A @a Token with the string to be converted.
	@return A newly allocated string which can be used internally.

	The use of this function is to remove the enclosing quotes and process
	any escape characters. A new string is allocated, which must be free'd
	after use.

	This function will not return if the memory required for the new string
	can not be allocated.
*/
char *processString(Token *string) {
	size_t writePosition = 0, readPosition, maxReadPosition, i;
	char *text;
	/* The string constant may be unterminated. In that case, return NULL to
	   notify the caller. */
	maxReadPosition = strlen(string->text);
	if (maxReadPosition <= 1 || string->text[maxReadPosition - 1] != '"') {
		return NULL;
	}
	maxReadPosition--;

	text = (char *) safeStrdup(string->text, "processString");
	readPosition = 1;
	while(readPosition < maxReadPosition) {
		if (text[readPosition] == '\\') {
			readPosition++;
			/* String may still contain error if last in text == \" !!!!! */
			if (readPosition == maxReadPosition) {
				free(text);
				return NULL;
			}
			switch(text[readPosition++]) {
				case '\n':	/* String continuation. Skip. */
					break;
				case 'n':
					text[writePosition++] = '\n';
					break;
				case 'r':
					text[writePosition++] = '\r';
					break;
				case '\'':
					text[writePosition++] = '\'';
					break;
				case '\\':
					text[writePosition++] = '\\';
					break;
				case 't':
					text[writePosition++] = '\t';
					break;
				case 'b':
					text[writePosition++] = '\b';
					break;
				case 'f':
					text[writePosition++] = '\f';
					break;
				case 'a':
					text[writePosition++] = '\a';
					break;
				case 'v':
					text[writePosition++] = '\v';
					break;
				case '?':
					text[writePosition++] = '\?';
					break;
				case '"':
					text[writePosition++] = '"';
					break;
				case 'x': {
					/* Hexadecimal escapes */
					int value = 0;
					/* Read at most two characters, or as many as are valid. */
					for (i = 0; i < 2 && (readPosition + i) < maxReadPosition && isxdigit(text[readPosition + i]); i++) {
						value <<= 4;
						if (isdigit(text[readPosition + i]))
							value += (int) (text[readPosition + i] - '0');
						else
							value += (int) (tolower(text[readPosition + i]) - 'a') + 10;
					}

					readPosition += i;

					/* Two things may have happened here:
					   - The escape had no hexadecimal characters following \x
					   - The escaped value was zero.
					   Either way, we don't want to keep this value.
					*/
					if (value == 0) {
						free(text);
						return NULL;
					}

					text[writePosition++] = (char) value;
					break;
				}
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7': {
					/* Octal escapes */
					int value = (int)(text[readPosition - 1] - '0');
					size_t maxIdx = text[readPosition - 1] < '4' ? 2 : 1;
					for (i = 0; i < maxIdx && readPosition + i < maxReadPosition && text[readPosition + i] >= '0' && text[readPosition + i] <= '7'; i++)
						value = value * 8 + (int)(text[readPosition + i] - '0');

					readPosition += i;

					/* A value of zero is unacceptable. */
					if (value == 0) {
						free(text);
						return NULL;
					}
					text[writePosition++] = (char) value;
					break;
				}
				default:
					text[writePosition++] = text[readPosition - 1];
					break;
			}
		} else {
			text[writePosition++] = text[readPosition++];
		}
	}
	/* Terminate string. */
	text[writePosition] = 0;
	return text;
}

/** Reentrant version of strtok
	@param string The string to tokenise.
	@param separators The list of token separators.
	@param state A user allocated character pointer.

	This function emulates the functionality of the Un*x function strtok_r.
	Note that this function destroys the contents of @a string.
*/
char *strtokReentrant(char *string, const char *separators, char **state) {
	char *retval;
	if (string != NULL)
		*state = string;

	/* Skip to the first character that is not in 'separators' */
	while (**state != 0 && strchr(separators, **state) != NULL) (*state)++;
	retval = *state;
	if (*retval == 0)
		return NULL;
	/* Skip to the first character that IS in 'separators' */
	while (**state != 0 && strchr(separators, **state) == NULL) (*state)++;
	if (**state != 0) {
		/* Overwrite it with 0 */
		**state = 0;
		/* Advance the state pointer so we know where to start next time */
		(*state)++;
	}
	return retval;
}

/** Split a string into a @a List of strings.
	@param string The string to split.
	@param separators The list of separators to use.
	@param keepEmpty Whether to include empty strings in the list.

	Note that this function destroys the contents of @a string.
*/
List *split(char *string, const char *separators, bool keepEmpty) {
	List *retval;

	retval = newList();
	if (keepEmpty) {
		char *state = string;
		while (*state != 0) {
			state = string;
			while (*state != 0 && strchr(separators, *state) == NULL) state++;
			if (state == string)
				listAppend(retval, NULL);
			else
				listAppend(retval, safeStrndup(string, state - string, "split"));
			string = state + 1;
		}
	} else {
		char *strtokState, *result;
		result = strtokReentrant(string, separators, &strtokState);

		while (result != NULL) {
			listAppend(retval, safeStrdup(result, "split"));
			strtokReentrant(NULL, separators, &strtokState);
		}
	}

	return retval;
}
