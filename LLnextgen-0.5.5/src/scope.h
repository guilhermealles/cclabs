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

#ifndef SCOPE_H
#define SCOPE_H

#include "bool.h"

#define HASHTABLE_SIZE 257

typedef struct Tuple {
	const char *key;
	void *data;
	struct Tuple *next;	/* Linked list */
} Tuple;

typedef struct {
	Tuple *table[HASHTABLE_SIZE];
} Scope;

Scope *newScope(void);
void *lookup(Scope *scope, const char *key);
bool declare(Scope *scope, const char *key, void *data);
void deleteScope(Scope *scope);
bool iterateScope(Scope *scope, void (*iterator)(const char *key, void *data, void *userData), void *userData);
#endif
