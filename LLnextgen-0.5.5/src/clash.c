/* Copyright (C) 2006,2008 G.P. Halkes
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
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "scope.h"
#include "clash.h"

static Scope *clashScope;

void initialiseClashScope(void) {
	const char *percent;
	int i;
	
	clashScope = newScope();
	
	for (i = 0; clashStrings[i] != NULL; i++) {
		if ((percent = strchr(clashStrings[i], '%')) != NULL) {
			char *expandedString;

			if (prefixDirective == NULL)
				continue;
			
			/* length of new string is length of clashStrings[i] - 2 (format spec)
			   + length prefix + 1 (nul byte) */
			expandedString = safeCalloc(strlen(clashStrings[i]) - 1 + strlen(prefix), "initialiseClashScope");
			strncpy(expandedString, clashStrings[i], percent - clashStrings[i]);
			strcat(expandedString, prefix);
			strcat(expandedString, percent + 2);
			/* Data = 2 indicates that the string in the scope should be free'd
			   when doing MEM_DEBUG */
			declare(clashScope, expandedString, (void *) 2);
		} else {
			/* Random data other than NULL so we can determine hits */
			declare(clashScope, clashStrings[i], (void *) 1);
		}
	}
	
}

bool checkClash(const char *string, int stringSelection) {
	/* All the items in the clashScope are problematic, but also anything that
	   matches LL_%u, <prefix>%u_%s or EOFILE. */
	unsigned int number;
	char ch;

	return (lookup(clashScope, string) != NULL || strcmp("EOFILE", string) == 0 ||
		/* Note: the %c is used to allow symbols which start with LL_%u, but are
		   not equal to it. There is no other way of doing this to my knowledge,
		   because sscanf will still report 1 if there are characters following
		   the last conversion.
		   Also note that suppressing assignment means that a conversion does not
		   count for the return value for sscanf. */
		sscanf(string, "LL_%u%c", &number, &ch) == 1 ||
		(strncmp(prefix, string, strlen(prefix)) == 0 && sscanf(string + strlen(prefix), "%u_%c", &number, &ch) == 2) ||
		((stringSelection & SEL_LLRETVAL) && strcmp(string, "LLretval") == 0));
}

#ifdef MEM_DEBUG
void freeClashScope(void) {
	iterateScope(clashScope, freeClashScopeStrings, NULL);
	deleteScope(clashScope);
}
#endif
