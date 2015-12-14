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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "scope.h"
#include "globals.h"

/** Calculate a hash value for a string.
	@param key The string to hash.
	@return The hash value associated with the string.
	
	This is not a very strong hash function, but using it will be better than
	simply using a list.
*/
static int hash(const char *key) {
	const char *ptr;
	int hashValue = 0;
	
	ptr = key;
	
	while (*ptr)
		hashValue += (int) *ptr++;
	
	return hashValue;
}

/** Allocate and initialise a new scope structure.
	@return A newly allocated @a Scope structure.

	If memory can not be allocated, the function will not return.
*/
Scope *newScope(void) {
	Scope *scope;
	scope = (Scope *) safeCalloc(sizeof(Scope), "newScope");
	return scope;
}

/** Add a symbol to the scope.
	@param scope The scope to add the symbol to.
	@param key The symbol.
	@param data The data to be associated with the symbol.

	If memory for the new data item cannot be allocated, the function will not
	return.
*/
static void addToScope(Scope *scope, const char *key, void *data) {
	Tuple *tuple;
	int hashValue;
	
	tuple = (Tuple *) safeMalloc(sizeof(Tuple), "addToScope");
	tuple->key = key;
	tuple->data = data;
	
	hashValue = hash(key) % HASHTABLE_SIZE;
	/* Each hash bucket contains a singly linked list. */
	tuple->next = scope->table[hashValue];
	scope->table[hashValue] = tuple;
}

/** Find a symbol's data in a scope.
	@param scope The scope to search.
	@param key The symbol to find.

	@return A pointer to the data associated with the symbol, or @a NULL if the
	symbol was not found.
*/
void *lookup(Scope *scope, const char *key) {
	Tuple *tuple;
	int hashValue;
	
	hashValue = hash(key) % HASHTABLE_SIZE;
	
	tuple = scope->table[hashValue];
	/* Search the singly linked list. */
	while (tuple != NULL && strcmp(key, tuple->key) != 0)
		tuple = tuple->next;
	
	/*@-usereleased@*/
	if (tuple != NULL)
		return tuple->data;
	/*@+usereleased@*/

	return NULL;
}

/** Declare a symbol in a scope.
	@param scope The scope to add the symbol to.
	@param key The symbol.
	@param data The data to be associated with the symbol.
	@return A boolean that indicates whether the @a declare action was
		successful.

	The difference from the @a addToScope function is that @a declare checks
	that the symbol does not already exist in the @a scope, while @a addToScope
	blindly adds the symbol.

	If memory for the new data item cannot be allocated, the function will not
	return.
*/
bool declare(Scope *scope, const char *key, void *data) {
	if (lookup(scope, key))
		return false;
	
	addToScope(scope, key, data);
	return true;
}

/** Delete a @a Scope and all its tuples.
	@param scope The @a Scope to delete.

	Note: this does not delete any data items refered to, only the tuples
	that refer to them.
*/
void deleteScope(Scope *scope) {
	int i;
	
	for (i = 0; i < HASHTABLE_SIZE; i++) {
		Tuple *tuple;
		while (scope->table[i] != NULL) {
			tuple = scope->table[i];
			scope->table[i] = tuple->next;
			free(tuple);
		}
	}
	free(scope);
}


/** Iterate over the elements of a @a Scope.
	@param scope The @a Scope to iterate over.
	@param iterator The function to call for each element.
	@return A boolean that indicates whether the @a iterator has been called.
*/
bool iterateScope(Scope *scope, void (*iterator)(const char *key, void *data, void *userData), void *userData) {
	bool iteratorCalled = false;
	int i;
	
	for (i = 0; i < HASHTABLE_SIZE; i++) {
		Tuple *tuple;
		tuple = scope->table[i];
		while (tuple != NULL) {
			iteratorCalled = true;
			iterator(tuple->key, tuple->data, userData);
			tuple = tuple->next;
		}
	}

	return iteratorCalled;
}
