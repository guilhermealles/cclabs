#ifndef INTSET_H

/* file:   intset.h
 * author: Arnold Meijster (a.meijster@rug.nl)
 * descr:  ADT that implements the standard set operations on
 *         sets of unsigned integers.
 */

typedef struct intSet {
  unsigned int size;    /* size of the array 'bits'                   */
  unsigned int *bits;   /* the set itself represented as a bit vector */
} intSet;

intSet makeEmptyIntSet(void);
intSet copyIntSet(intSet s);
void freeIntSet(intSet s);
int isEmptyIntSet(intSet s);
void insertIntSet(unsigned int n, intSet *s);
void deleteIntSet(unsigned int n, intSet *s);
int isMemberIntSet(unsigned int n, intSet s);
void unionIntSet(intSet *lhs, intSet rhs);
void intersectionIntSet(intSet *lhs, intSet rhs);
int isSubIntSet(intSet lhs, intSet rhs);
int isEqualIntSet(intSet lhs, intSet rhs);
int isDisjointIntSet(intSet lhs, intSet rhs);
unsigned int chooseFromIntSet(intSet s);
void fprintIntSet(FILE *f, intSet s);
void fprintlnIntSet(FILE *f, intSet s);
void printIntSet(intSet s);
void printlnIntSet(intSet s);
intSet readIntSetFromFile(FILE *f);

#endif /* INTSET_H */
