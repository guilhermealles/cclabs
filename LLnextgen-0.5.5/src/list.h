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

#ifndef LIST_H
#define LIST_H

typedef struct {
	int fill, maxFill;
	void **data;
} List;


List *newList(void);
void listAppend(List *list, void *data);
void listPrepend(List *list, void *data);
void *listPop(List *list);
void *listIndex(List *list, int idx);
int listSize(List *list);
void listMerge(List *dest, List *src);
void deleteList(List *list);
void deleteListWithContents(List *list);
void listClear(List *list);
void listDelete(List *list, int idx);
void listReplace(List *list, int idx, void *data);
void listInsert(List *list, int idx, void *data);
#endif
