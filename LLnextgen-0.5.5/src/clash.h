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

#ifndef CLASH_H
#define CLASH_H

#define SEL_NONE (0)
#define SEL_LLRETVAL (1<<0)

void initialiseClashScope(void);
bool checkClash(const char *string, int stringSelection);

#ifdef MEM_DEBUG
void freeClashScopeStrings(const char *key, void *data, UNUSED void *userData);
void freeClashScope(void);
#endif

extern const char *clashStrings[];

#endif
