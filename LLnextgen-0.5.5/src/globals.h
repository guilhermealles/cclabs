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

#ifndef GLOBALS_H
#define GLOBALS_H

/* This should be automatically determined from the repository
   [and is, by the mkdist.sh script] */
#define VERSION_STRING "0.5.5"
/* Version number in HEX. Must be defined to current version to allow self
   compilation. Will be set to correct value by mkdist.sh script. Used to
   create a define in the output such that users can #ifdef for features. */
#define VERSION_HEX "0x000505L"

#include <stdio.h>
#include "nonRuleAnalysis.h"
#include "list.h"

/* Variables */
extern bool errorSeen, softErrorSeen;

/* Functions */

/* Allocate a block of memory, and die if it cannot be allocated */
void *safeMalloc(size_t size, const char *where);
/* Is the same as safeMalloc, but also clears the memory block */
void *safeCalloc(size_t size, const char *where);
void *safeRealloc(void *ptr, size_t size, const char *where);
char *safeStrdup(const char *orig, const char *where);
char *safeStrndup(const char *orig, size_t length, const char *where);
void safeStrcatWithSpace(char **base, const char *append, const char *where);
FILE *safeFopen(const char *name, const char *mode);

#define WARNING_UNMASKED (0)
#define WARNING_ARG_SEPARATOR (1<<0)
#define WARNING_OPTION_OVERRIDE (1<<1)
#define WARNING_UNBALANCED_C (1<<2)
#define WARNING_MULTIPLE_PARSER (1<<3)
#define WARNING_EOFILE (1<<4)
#define WARNING_UNUSED (1<<5)
#define WARNING_DATATYPE (1<<6)
#define WARNING_DISCARD_RETVAL (1<<6)

/* If gcc is used, it can check whether the correct types are used for the
   arguments after the format string. Furthermore we can tell it about certain
   functions that do not return (i.e. call exit or abort) such that it won't
   complain about not returning values elsewhere. To this end we define the
   GCC_ATTRIBUTE macro. */
#ifdef __GNUC__
#define GCC_ATTRIBUTE(x) __attribute__((x))
#define UNUSED __attribute__((unused))
#else
#define GCC_ATTRIBUTE(x)
#define UNUSED
#endif

/*@noreturn@*/ void fatal(const char *fmt, ...) GCC_ATTRIBUTE(format(printf, 1, 2)) GCC_ATTRIBUTE(noreturn);
/*@noreturn@*/ void fail(const char *fmt, ...) GCC_ATTRIBUTE(format(printf, 1, 2)) GCC_ATTRIBUTE(noreturn);
void error(Token *token, const char *fmt, ...) GCC_ATTRIBUTE(format(printf, 2, 3));
void softError(Token *token, const char *fmt, ...) GCC_ATTRIBUTE(format(printf, 2, 3));
void warning(int warningType, Token *token, const char *fmt, ...) GCC_ATTRIBUTE(format(printf, 3, 4));
void generalWarning(int warningType, const char *fmt, ...) GCC_ATTRIBUTE(format(printf, 2, 3));
void continueMessage(const char *fmt, ...) GCC_ATTRIBUTE(format(printf, 1, 2));


void printAt(Token *token);
void endMessage(void);

#define PANIC() fail("Program failure in file %s on line %d.\nPlease see the section Bugs in the manual.\n", __FILE__, __LINE__)
#ifdef DEBUG
#define PANICMSG(msg) fail("%s:%d: %s", __FILE__, __LINE__, msg)
#else
#define PANICMSG(msg) PANIC()
#endif
#define ASSERT(cond) do { if (!(cond)) PANICMSG("Assertion (" #cond ") failed\n"); } while (0)

char *processString(Token *file);
char *strtokReentrant(char *string, const char *separators, char **state);
List *split(char *string, const char *separators, bool keepEmpty);

/* Don't want to bother making a header file just for this. */
void freeMemory(void);

#define uDeclaration un.declaration
#define uDirective un.directive
#define uCode un.code
#define uToken un.token
#define uLiteral un.literal
#define uNumber un.number
#define uNonTerminal un.nonTerminal
#define uTerminal un.terminal
#define uTerm un.term
#define uAction un.action
#define uNext un.next
#define uPtr un.ptr

#endif

