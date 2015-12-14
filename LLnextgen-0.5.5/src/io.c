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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include "globals.h"
#include "io.h"
#include "option.h"
#define LL_NOTOKENS
#include "grammar.h"

/* Add cast to void to suppress "value computed not used" warnings by gcc */
#ifdef __GNUC__
#define CAST_VOID (void)
#else
#define CAST_VOID
#endif

/** Open a File.
	@param name The path/file name of the file to open.
	@param mode The mode string (same as for @a fopen).
	@param traceLines A boolean indicating whether to trace the line number when
		writing the @a File.
	@return A pointer to a @a File struct, or NULL if the file could not be
		opened.

	If memory for the struct can not be allocated, the program is aborted.
*/
static File *openFile(const char *name, const char *mode, bool traceLines) {
	File *retval;
	retval = (File *) safeMalloc(sizeof(File), "openFile");
	retval->stream = fopen(name, mode);
	if (retval->stream == NULL) {
		free(retval);
		return NULL;
	}
	retval->line = 1;
	retval->traceLines = traceLines && !option.dontGenerateLineDirectives;
	retval->name = name;
	return retval;
}

/** Reopen a @a File with a different mode.
	@param file The @a File to re-open.
	@param mode The new mode to open the file with.
	@return a boolean indicating whether the operation succeeded.

	If the operation fails, the old file will not be available anymore.
*/
static bool reopenFile(File *file, const char *mode) {
	file->stream = freopen(file->name, mode, file->stream);
	return file->stream != NULL;
}

/** Open a file and check that it was generated by LLnextgen.
	@param name The path/file name of the file to open.
	@param generateLineDirectives Boolean to indicate whether #line-directives
		should be generated in the output. This is mapped onto the traceLines
		parameter of openFile.
	@return a @a File struct for the requested file, opened in write mode.

	The program is aborted if the file is not ours, or could not be opened for
	writing.
*/
File *checkAndOpenFile(const char *name, bool generateLineDirectives) {
	char lineBuffer[sizeof(IDENTIFIER_STRING)];
	File *retval;

	/* First check if the file exists, by opening for reading. */
	retval = openFile(name, "r", generateLineDirectives);

	/* If the file exists, check for the comment at the start of the file. */
	if (retval != NULL) {
		/* If the file is shorter than the expected string, it can't be ours. */
		if (fread(lineBuffer, 1, sizeof(IDENTIFIER_STRING) - 1, retval->stream) != sizeof(IDENTIFIER_STRING) - 1)
			fatal("File %s is not a file written by LLnextgen. Aborting\n", name);
		lineBuffer[sizeof(IDENTIFIER_STRING) - 1] = 0;
		if (strcmp(IDENTIFIER_STRING, lineBuffer) != 0)
			fatal("File %s is not a file written by LLnextgen. Aborting\n", name);
		/* As we have established that it is our file, re-open it for writing. */
		if (!reopenFile(retval, "w"))
			fatal("Could not open file %s for writing: %s\n", name, strerror(errno));
	} else {
/* ENOENT is NOT in ANSI C. Therefore, we need to #ifdef it to allow
   compilation on platforms that don't have it (don't know of any that don't
   though). */
#ifdef ENOENT
		/* If the file was there but we couldn't open it, print an error. */
		if (errno != ENOENT)
			fatal("Could not open file %s for reading: %s\n", name, strerror(errno));
#endif
		/* If we couldn't open it because it wasn't there, open it now */
		if ((retval = openFile(name, "w", generateLineDirectives)) == NULL)
			fatal("Could not open file %s for writing: %s\n", name, strerror(errno));
	}
	/* Make sure the file contains our identifier string. */
	tfprintf(retval, IDENTIFIER_STRING);
	return retval;
}

/** Count the number of lines a string consists of.
	@param string The string to consider.
	@return The number of lines the string consists of, starting from 0.
*/
static int countLines(const char *string) {
	size_t length, i;
	int line = 0;

	length = strlen(string);
	for (i = 0; i < length; i++)
		if (string[i] == '\n')
			line++;
	return line;
}

/** Count the number of lines a substring consists of.
	@param string The string to consider.
	@param length The number of characters to consider.
	@return The number of lines the substring consists of, starting from 0.
*/
static int countLinesN(const char *string, size_t length) {
	size_t i;
	int line = 0;

	for (i = 0; i < length; i++)
		if (string[i] == '\n')
			line++;
	return line;
}

/** Formatted output to a @a File.
	@param file The @a File to print to.
	@param format The format string. See printf(3) for details.
	@param ... Arguments to be converted.
	@return The number of characters printed.

	The @a tfprintf function is a wrapper around @a vfprintf that counts the
	number of lines that are printed. This is to allow #line directives in the
	output to refer to the file itself. The reason we want to do this, is that
	if there is an error in the generated code we want to report it in the .c
	file, while errors in the user's code should be reported relative to the
	.g file(s).
*/
int tfprintf(File *file, const char *format, ...) {
	va_list args;
	int retval;

	va_start(args, format);

	/* Only check the arguments if we need to trace the lines */
	if (file->traceLines) {
		bool isLong;
		const char *conversion = format;
		va_list tmpArgs;
		int precision;

		file->line += countLines(format);

		va_start(tmpArgs, format);
		while ((conversion = strchr(conversion, '%')) != NULL) {
			isLong = false;
			precision = -1;
			/* NOTE: we assume the compiler/programmer has checked the input
			   such that the format string is actually valid. This means we
			   don't check for things like %hld. We do some sanity checking to
			   make sure we don't do stuff that violates the ANSI C standard
			   though. */
			conversion++;
			while (*conversion != 0 && strchr("diouxXeEfFgGaAcspn%", *conversion) == NULL) {
				switch (*conversion) {
					case 'h':
						break;
					case 'l':
					case 'L':
						isLong = true;
						break;
					case 'j':
					case 't':
					case 'q':
					case 'z':
						PANICMSG("Unsupported conversion length modifier passed to tfprintf\n");
					case '*':
						/* WARNING: This code is untested! */
						precision = va_arg(tmpArgs, int);
						break;
					case '.': {
						/* WARNING: This code is untested! */
						char *endptr;
						long value;
						errno = 0;
						value = strtol(conversion + 1, &endptr, 10);
						if (value <= 0 || value > INT_MAX || errno != 0)
							PANICMSG("Faulty precision passed to tfprintf\n");
						conversion += endptr - conversion;
						break;
					}
					default:
						break;
				}
				conversion++;
			}
			/* Get arguments from the list, and count lines in %s conversions. */
			switch (*conversion) {
				case 's':
					if (precision > 0)
						file->line += countLinesN(va_arg(tmpArgs, char *), (size_t) precision);
					else
						file->line += countLines(va_arg(tmpArgs, char *));
					break;
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
				case 'c':
					if (isLong)
						CAST_VOID va_arg(tmpArgs, long);
					else
						CAST_VOID va_arg(tmpArgs, int);
					break;
				case 'e':
				case 'E':
				case 'f':
				case 'F':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					if (isLong)
						CAST_VOID va_arg(tmpArgs, long double);
					else
						CAST_VOID va_arg(tmpArgs, double);
					break;
				case 'p':
				case 'n':
					CAST_VOID va_arg(tmpArgs, void *);
					break;
				case '%':
					break;
				default:
					PANIC();
			}
		}
		va_end(tmpArgs);
	}
	/* Now do the actual printing. */
	retval = vfprintf(file->stream, format, args);
	va_end(args);
	return retval;
}

/** Write the first part of a string to file.
	@param file The @a File to write to.
	@param string The string to write.
	@param n The number of leading characters to write.
	@return 0 for success or -1 if not all characters could be writen.
*/
int tfputsn(File *file, const char *string, size_t n) {
	if (file->traceLines)
		file->line += countLinesN(string, n);

	if (fwrite(string, 1, n, file->stream) != n)
		return -1;
	return 0;
}

/** Write a string to file.
	@param file The @a File to write to.
	@param string The string to write.
	@return A value >= 0 for success or EOF if not all characters could be writen.
*/
int tfputs(File *file, const char *string) {
	if (file->traceLines)
		file->line += countLines(string);
	return fputs(string, file->stream);
}

/** Write a character to file.
	@param file The @a File to write to.
	@param c The character to write.
	@return A value >= 0 for success or EOF if not all characters could be writen.
*/
int tfputc(File *file, int c) {
	if (file->traceLines && c == '\n')
		file->line++;
	return fputc(c, file->stream);
}

/** Write a line directive in a file.
	@param file The @a File to write to.
	@param code The @a Code to write a directive for, or @a NULL to write a
		line directive referencing the output file.
	@return The number of characters writen.
*/
int tfprintDirective(File *file, CCode *code) {
	if (!file->traceLines)
		return 0;
	file->line++;
	if (code != NULL)
		return fprintf(file->stream, "#line %d \"%s\"\n", code->lineNumber, code->fileName);
	return fprintf(file->stream, "#line %d \"%s\"\n", file->line, file->name);
}

/** Write a comment in the output indicating the token that an int represents.
	@param file The @a File to write to.
	@param token The condensed token number to write a comment for.
	@return The number of characters writen.
*/
int tfprintTokenComment(File *file, int token) {
	/* Doing a test for traceLines here is slower than just doing the increment */
	file->line++;
	if (condensedToTerminal[token].flags & CONDENSED_ISTOKEN)
		return fprintf(file->stream, "/* %s */\n", condensedToTerminal[token].uToken->token[0]->text);
	return fprintf(file->stream, "/* %s */\n", LLgetSymbol(condensedToTerminal[token].uLiteral));
}

/** Close a file and free associated resources.
	@param file The @a File to close.
	@param freeName Boolean indicating whether to also free the @a name member.
*/
void closeFile(File *file, bool freeName) {
	fclose(file->stream);
	if (freeName) {
		/* Trick to shut up compilers about casting a const pointer to a
		   non-const pointer. Will be optimised out anyway. */
		union {
			const char *cptr;
			char *ptr;
		} compilerHack;

		compilerHack.cptr = file->name;
		free(compilerHack.ptr);
	}
	free(file);
}
