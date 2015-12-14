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
#include <string.h>

#include "set.h"
#include "globals.h"

/** Allocate a new bit set of specified size.
	@param items The maximum number of items that will be used in the bit set.
	@return A new bit set.
	
	If memory cannot be allocated for the bit set, the function will not return.
*/
set newSet(int items) {
	set retval;

	/*@+matchanyintegral@*/
	retval.size = (items + (8 * sizeof(int)) - 1) / (8 * sizeof(int));
	/*@-matchanyintegral@*/
	retval.bits = (int *) safeCalloc(retval.size * sizeof(int), "newSet");
	return retval;
}

/*@-usereleased@*/
/** Delete a bit set.
	@param a The set to be deleted.
*/
void deleteSet(set a) {
	free(a.bits);
}
/*@+usereleased@*/

/** Calculate the set union.
	@param a The first set to operate on, and also where the result is stored.
	@param b The second set to operate on.
	@return A boolean to indicate whether the operation has changed set @a a.
*/
bool setUnion(set a, const set b) {
	int i;
	
	if (a.size != b.size)
		PANICMSG("Taking union of unequal sized sets is not allowed\n");
	
	/* skip all "equal" parts */
	for (i = 0; i < a.size && !(~a.bits[i] & b.bits[i]); i++) {}
	/* check whether we've skipped through the set, or stopped because
		we found a difference */
	if (i != a.size) {
		for (; i < a.size; i++)
			a.bits[i] |= b.bits[i];
		return true;
	}
	return false;
}

/** Calculate the set intersection.
	@param a The first set to operate on, and also where the result is stored.
	@param b The second set to operate on.
	@return A boolean to indicate whether the operation has changed set @a a.
*/
bool setIntersection(set a, const set b) {
	int i;
	
	if (a.size != b.size)
		PANICMSG("Taking intersection of unequal sized sets is not allowed\n");
	
	/* skip all "equal" parts */
	for (i = 0; i < a.size && !(a.bits[i] & ~b.bits[i]); i++) {}
	/* check whether we've skipped through the set, or stopped because
		we found a difference */
	if (i != a.size) {
		for (; i < a.size; i++)
			a.bits[i] &= b.bits[i];
		return true;
	}
	return false;
}

/** Determine if the intersection of two sets is empty.
	@param a The first set to operate on.
	@param b The second set to operate on.
	@return A boolean indicating whether the intersection of @a a and @a b is
		empty.
*/
bool setIntersectionEmpty(const set a, const set b) {
	int i;

	if (a.size != b.size)
		PANICMSG("Taking intersection of unequal sized sets is not allowed\n");
	/* skip all empty intersection parts */
	for (i = 0; i < a.size && !(a.bits[i] & b.bits[i]); i++) {}
	/* check whether we've skipped through the set, or stopped because
		we found a non-empty intersection */
	if (i != a.size)
		return false;
	return true;
}

/** Set a bit in a bit set.
	@param a The bit set in which to set the bit. 
	@param i The bit number to set.
	@return A boolean to indicate whether the operation has changed set @a a.
*/
bool setSet(set a, int i) {
	int bit = 1<<(i % (sizeof(int) * 8));
	/*@+matchanyintegral@*/
	int word = i / (sizeof(int) * 8);
	/*@-matchanyintegral@*/
	
	if (!(a.bits[word] & bit)) {
		a.bits[word] |= bit;
		return true;
	}
	return false;
}

/** Determine if a specific bit is set in a bit set.
	@param a The set in which to find the bit.
	@param i The bit number to test
	@return A boolean indicating whether the specified bit is set.
*/
bool setContains(const set a, int i) {
	int bit = 1<<(i % (sizeof(int) * 8));
	/*@+matchanyintegral@*/
	int word = i / (sizeof(int) * 8);
	/*@-matchanyintegral@*/
	
	if (a.bits[word] & bit) {
		return true;
	}
	return false;
}

/** Copy the contents of one bit set to another.
	@param a The destination.
	@param b The source.
*/
void setCopy(set a, const set b) {
	int i;
	
	if (a.size != b.size)
		PANICMSG("Copying of unequal sized sets is not allowed\n");
	
	for (i = 0; i < a.size; i++)
		a.bits[i] = b.bits[i];
}

/** Reset all the bits in a set.
	@param a The set to clear.
*/
void setClear(set a) {
	memset(a.bits, 0, a.size * sizeof(int));
}

/** Calculate the set difference of two sets.
	@param a The first set to operate on, and also where the result is stored.
	@param b The second set to operate on.
	@return A boolean to indicate whether the operation has changed set @a a.
*/
bool setMinus(set a, const set b) {
	int i;
	
	if (a.size != b.size)
		PANICMSG("Subtracting of unequal sized sets is not allowed\n");
	
	/* skip all "equal" parts */
	for (i = 0; i < a.size && !(a.bits[i] & b.bits[i]); i++) {}
	/* check whether we've skipped through the set, or stopped because
		we found a difference */
	if (i != a.size) {
		for (; i < a.size; i++)
			a.bits[i] &= ~b.bits[i];
		return true;
	}
	return false;
}

/** Determine whether all bits in a bit set are reset.
	@param a The set to scan.
	@return A boolean indicating whether all the bits in the bit set are reset.
*/
bool setEmpty(const set a) {
	int i;
	
	for (i = 0; i < a.size && a.bits[i] == 0; i++) {}
	if (i != a.size)
		return false;
	return true;
}

/** Determine if two sets are equal.
	@param a The first set to operate on.
	@param b The second set to operate on.
	@return A boolean indicating whether the set @a a and @a b are equal.
*/
bool setEquals(const set a, const set b) {
	int i;
	
	if (a.size != b.size)
		PANICMSG("Comparision of unequal sized sets is not allowed\n");
	for (i = 0; i < a.size; i++)
		if (a.bits[i] != b.bits[i])
			return false;
	return true;
}

/** Determine the number of bits that are set in a bit set.
	@param a The set to consider.
	@return The number of bits set in @a a.
*/
int setFill(const set a) {
	int i, value, fill = 0;
	for (i = 0; i < a.size; i++) {
		value = a.bits[i];
		while (value) {
			value &= value - 1;
			fill++;
		}
	}
	return fill;
}

/** Find the first set bit in a bit set.
	@param a The set to consider.
	@return The index of the first bit that is set in @a a.
*/
int setFirstToken(const set a) {
	int i, j = 0;
	
	/* Skip empty part */
	for (i = 0; i < a.size && a.bits[i] == 0; i++) {}
	
	while (!(a.bits[i] & (1<<j))) j++;
	return (int)(i * sizeof(int) * 8 + j);
}

int numberOfSets = 0;
static int setListFill = 0;
set *setList = NULL;

/** Find the index associated with a set.
	@param a The set to find an index for.
	@param create Bool to indicate whether a new index may be used.

	This function is used to enumerate all the sets that will be used in the
	output, as well as to find the number of a previously enumerated set. If
	create is @a false and the set has not been enumerated yet, the function
	will execute a @a PANIC.
*/
int setFindIndex(const set a, bool create) {
	int i;
	
	for (i = 0; i < numberOfSets; i++)
		if (setEquals(a, setList[i]))
			return i;
	
	/* If a set is requested but it cannot be created, we cannot continue. */
	if (!create)
		PANIC();
	
	if (numberOfSets >= setListFill) {
		setListFill += 32;
		setList = (set *) safeRealloc(setList, setListFill * sizeof(set), "setFindIndex");
	}
		
	setList[numberOfSets] = a;
	return numberOfSets++;
}
