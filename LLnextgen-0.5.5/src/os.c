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

#include "bool.h"
#include "os.h"

#ifdef __WIN32__
#include <string.h>
#include <ctype.h>

/** Find the last occurence of any of a string of characters in a string.
	@param s The string to search.
	@param accept The string of characters to search for.

	This is the 'reverse' function of the ANSI C strpbrk function. Instead of
	searching for the first occurence it searches for the last occurence. This
	function is however not in ANSI C, so we have to implement is ourselves. It
	is only needed on Windows machines, as Windows filenames can have either
	/ or \ as the directory separator. This means we can't use strrchr.

	The arguments are expected to be valid strings.
*/
const char *strpbrkReverse(const char *s, const char *accept) {
	const char *lastMatch = NULL;
	while (*s != 0) {
		if (strchr(accept, *s) != NULL)
			lastMatch = s;
		s++;
	}
	return lastMatch;
}

/** Retrieve a pointer to the base of the name.
	@param name File name.
	
	This function takes Windows' case-insensitiviy into account as well as
	possible drive letter prefixes.
*/
const char *baseName(const char *name) {
	const char *slash;
	/* If there is a directory separator, that's where the basename starts 
	   Note that on Windows you may also use '/' as directory separator */
	slash = strpbrkReverse(name, "\\/");
	if (slash != NULL)
		return slash + 1;
	/* If the second character is a colon, the name starts with a drive letter.
	   Windows does not allow colons in names. */
	if (strlen(name) >= 2 && name[1] == ':')
		return name + 2;
	return name;
}

/** Compare two strings without regard for case.
	@param a First string.
	@param b Second string.
*/
int strcmpCaseInsensitive(const char *a, const char *b) {
	char la, lb;
	/* Only stop when both characters are nul. When only one of them is nul,
	   the comparisons below will do a return. */
	while (!(*a == 0 && *b == 0)) {
		la = tolower(*a++);
		lb = tolower(*b++);
		if (la < lb)
			return -1;
		else if (la > lb)
			return 1;
	}
	return 0;
}

/** Compare the executable's name with "LLgen".
	@param name Executable's name.

	This function takes Windows' case-insensitivity into account as well as
	the .exe suffix problem, and possible drive letter prefixes.
*/
bool isLLgen(const char *name) {
	name = baseName(name);
	if (strcmpCaseInsensitive(name, "LLgen") == 0)
		return true;
	return strcmpCaseInsensitive(name, "LLgen.exe") == 0;
}

#else
#include <string.h>

/** Retrieve a pointer to the base of the name.
	@param name File name.
*/
const char *baseName(const char *name) {
	const char *slash;
	/* If there is a directory separator, that's where the basename starts */
	slash = strrchr(name, '/');
	if (slash != NULL)
		return slash + 1;
	return name;
}

/** Compare the executable's name with "LLgen".
	@param name Executable's name.
*/
bool isLLgen(const char *name) {
	return strcmp(baseName(name), "LLgen") == 0;
}

#endif
