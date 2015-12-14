/* Copyright (C) 2005-2008 G.P. Halkes
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
#include <string.h>

#include "list.h"
#include "globals.h"

#define INITIAL_SIZE 32

/** Allocate a new @a List structure.
	@return The newly allocated @a List structure.
	
	The function will not return if memory cannot be allocated.
*/
List *newList(void) {
	List *list;
	
	list = (List *) safeMalloc(sizeof(List), "newList");
	list->data = (void **) safeCalloc(INITIAL_SIZE * sizeof(void *), "newList");
	list->fill = 0;
	list->maxFill = INITIAL_SIZE;
	
	return list;
}

/** Append a pointer to data to a list.
	@param list The @a List to add to.
	@param data The pointer to data to add.
	
	The function will not return if memory cannot be allocated for enlarging
	the internal array.
*/
void listAppend(List *list, void *data) {
	if (list->fill == list->maxFill) {
		list->maxFill *= 2;
		list->data = (void **) safeRealloc(list->data, list->maxFill * sizeof(void *), "listAppend");
		/* Note: fill is the old value of maxFill at this point */
		memset(list->data + list->fill, 0, sizeof(void *) * list->fill);
	}
	list->data[list->fill++] = data;
}

/** Prepend a pointer to data to a list.
	@param list The @a List to add to.
	@param data The pointer to data to add.
	
	The function will not return if memory cannot be allocated for enlarging
	the internal array.
*/
void listPrepend(List *list, void *data) {
	if (list->fill == list->maxFill) {
		list->maxFill *= 2;
		list->data = (void **) safeRealloc(list->data, list->maxFill * sizeof(void *), "listPrepend");
		/* Note: fill is the old value of maxFill at this point */
		memset(list->data + list->fill, 0, sizeof(void *) * list->fill);
	}
	memmove(list->data + 1, list->data, list->fill * sizeof(void *));
	list->data[0] = data;
	list->fill++;
}

/** Get the last item in the list, and remove it from the list.
	@param list The @a List to retrieve the item from.
	@return The last item from the list, or @a NULL if the list is empty.
*/
void *listPop(List *list) {
	void *retval;
	if (list->fill <= 0)
		return NULL;
	retval = list->data[--list->fill];
	list->data[list->fill] = NULL;
	return retval;
}

/** Get the item at an index from a list.
	@param list The @a List to retrieve the item from.
	@param idx The index from which to retrieve the item.
	@return The requested item from the list.

	The function does not return if an invalid element is requested.
*/
void *listIndex(List *list, int idx) {
	if (idx < 0 || idx >= list->fill)
		PANICMSG("List index out of bounds\n");
	return list->data[idx];
}

/** Get the size of a list.
	@param list The @a List to retrieve the size for.
*/
int listSize(List *list) {
	return list->fill;
}

/** Merge two lists into one.
	@param dest The first @a List to merge, and also the list where the result
		is stored.
	@param src The second @a List to merge.

	The data of @a src is appended to @a dest, but NOT removed from @a src.
*/
void listMerge(List *dest, List *src) {
	if (dest->fill + src->fill > dest->maxFill) {
		do {
			dest->maxFill *= 2;
		} while (dest->fill + src->fill > dest->maxFill);
		dest->data = (void **) safeRealloc(dest->data, dest->maxFill * sizeof(void *), "listMerge");
		memset(dest->data + dest->fill, 0, sizeof(void *) * (dest->maxFill - dest->fill));
	}
	memcpy(dest->data + dest->fill, src->data, src->fill * sizeof(void *));
	dest->fill += src->fill;
}

/** Delete a @a List.
	@param list The @a List to delete.

	Both the internal array and the @a List struct are free'd.
*/
void deleteList(List *list) {
	ASSERT(list != NULL);
	free(list->data);
	free(list);
}

/** Delete a @a List and waht the pointers point to.
	@param list The @a List to delete.
*/
void deleteListWithContents(List *list) {
	int i;

	ASSERT(list != NULL);
	for (i = 0; i < list->fill; i++)
		free(list->data[i]);

	deleteList(list);
}

/** Remove all items from a @a List.
	@param list The @a List to clear.

	The internal array is not free'd, nor is it resized.
*/
void listClear(List *list) {
	memset(list->data, 0, sizeof(void *) * list->maxFill);
	list->fill = 0;
}

/** Delete an element from a @a List.
	@param list The @a List to delete the item from.
	@param idx The index of the item to delete.
*/
void listDelete(List *list, int idx) {
	if (idx < 0 || idx >= list->fill)
		PANICMSG("List index out of bounds\n");
	memcpy(list->data + idx, list->data + idx + 1, sizeof(void *) * (list->fill - idx - 1));
	list->fill--;
}

/** Replace an item in @a List.
	@param list The @a List in which to replace the item.
	@param idx The index of the item to replace.
	@param data The data to place at @a idx.
*/
void listReplace(List *list, int idx, void *data) {
	if (idx < 0 || idx >= list->fill)
		PANICMSG("List index out of bounds\n");
	list->data[idx] = data;
}

/** Insert a pointer to data in a @a List.
	@param list The @a List to add to.
	@param idx The index to place the item at.
	@param data The pointer to data to add.
	
	The function will not return if memory cannot be allocated for enlarging
	the internal array.
*/
void listInsert(List *list, int idx, void *data) {
	if (idx < 0 || idx > list->fill)
		PANICMSG("List index out of bounds\n");
	if (list->fill == list->maxFill) {
		list->maxFill *= 2;
		list->data = (void **) safeRealloc(list->data, list->maxFill * sizeof(void *), "listInsert");
		/* Note: fill is the old value of maxFill at this point */
		memset(list->data + list->fill, 0, sizeof(void *) * list->fill);
	}
	memmove(list->data + idx + 1, list->data + idx, (list->fill - idx) * sizeof(void *));
	list->data[idx] = data;
	list->fill++;
}
