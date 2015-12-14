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

#ifndef SET_H
#define SET_H

#include "bool.h"

typedef struct {
	int size;
	int *bits;
} set;

set newSet(int items);
void deleteSet(set a);
bool setUnion(set a, const set b);
bool setIntersection(set a, const set b);
bool setIntersectionEmpty(const set a, const set b);
bool setSet(set a, int i);
bool setContains(const set a, int i);
void setCopy(set a, const set b);
void setClear(set a);
bool setMinus(set a, const set b);
bool setEmpty(const set a);
bool setEquals(const set a, const set b);
int setFill(const set a);
int setFirstToken(const set a);
int setFindIndex(const set a, bool create);

extern int numberOfSets;
extern set *setList;

#endif
