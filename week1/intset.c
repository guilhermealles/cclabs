/* file:   intset.c
 * author: Arnold Meijster (a.meijster@rug.nl)
 * descr:  ADT that implements the standard set operations on
 *         sets of unsigned integers.
 */

#include <stdio.h>
#include <stdlib.h>
#include "intset.h"

#define BITS_UINT (8*sizeof(unsigned int))

static unsigned int mask(unsigned int n) {
    /* returns 2 to the power n */
    return 1u << n;
}

static unsigned int minimum(unsigned int a, unsigned int b) {
    return (a < b ? a : b);
}

static unsigned int maximum(unsigned int a, unsigned int b) {
    return (a < b ? a : b);
}

static char peekChar(FILE *f) {
    char c;
    do {
        c = fgetc(f);
    } while ((c == ' ') || (c == '\t') || (c == '\n'));
    ungetc(c, f);
    return c;
}

static void expectCharacter(FILE *f, char expect) {
    char c = peekChar(f);
    if (c != expect) {
        fprintf(stderr, "Fatal error in readIntSetFromFile(): "
                "expected '%c'\n", expect);
        exit(EXIT_FAILURE);
    }
    fgetc(f);
}

static void resize(unsigned int sz, intSet *s) {
    if (sz > s->size) {
        int i;
        s->bits = realloc(s->bits, sz*sizeof(unsigned int));
        if (s->bits == NULL) {
            fprintf(stderr, "Fatal error: memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        for (i=s->size; i < sz; i++) {
            s->bits[i] = 0;
        }
        s->size = sz;
    }
}

intSet makeEmptyIntSet() {
    intSet s;
    s.size = 0;
    s.bits = NULL;
    return s;
}

intSet copyIntSet(intSet s) {
    intSet cp;
    int i;
    cp = makeEmptyIntSet();
    resize(s.size, &cp);
    for (i = 0; i < s.size; i++) {
        cp.bits[i] = s.bits[i];
    }
    return cp;
}

void freeIntSet(intSet s) {
    free(s.bits);
}

int isEmptyIntSet(intSet s) {
    unsigned int i = 0;
    while (i < s.size && s.bits[i] == 0) {
        i++;
    }
    return (i == s.size ? 1 : 0);
}

void insertIntSet(unsigned int n, intSet *s) {
    unsigned int idx = n / BITS_UINT;
    unsigned int m = mask(n % BITS_UINT);
    resize(idx+1, s);
    s->bits[idx] |= m;
}

void deleteIntSet(unsigned int n, intSet *s) {
    unsigned int m, idx = n / BITS_UINT;
    if (idx >= s->size) {
        return;  /* n is not a member of s */
    }
    m = mask(n % BITS_UINT);
    s->bits[idx] &= ~m;
}

int isMemberIntSet(unsigned int n, intSet s) {
    unsigned int idx = n / BITS_UINT;
    unsigned int m = mask(n % BITS_UINT);
    if (idx >= s.size) {
        return 0;  /* n is not a member of s */
    }
    return (s.bits[idx] & m ? 1 : 0);
}

void unionIntSet(intSet *lhs, intSet rhs) {
    unsigned int i;
    if (lhs->size <= rhs.size) {
        resize(rhs.size, lhs);
    }
    for (i=0; i < rhs.size; i++) {
        lhs->bits[i] |= rhs.bits[i];
    }
}

void intersectionIntSet(intSet *lhs, intSet rhs) {
    unsigned int i, sz = minimum(lhs->size, rhs.size);
    for (i=0; i < sz; i++) {
        lhs->bits[i] &= rhs.bits[i];
    }
    for (i=sz; i < lhs->size; i++) {
        lhs->bits[i] = 0;
    }
}

int isSubIntSet(intSet lhs, intSet rhs) {
    int i = 0;
    if (lhs.size < rhs.size) {
        while ((i < lhs.size) && (lhs.bits[i] == (lhs.bits[i] & rhs.bits[i]))) {
            i++;
        }
        return (i == lhs.size ? 1 : 0);
    }
    /* rhs.size <= lhs.size */
    while ((i < rhs.size) && (lhs.bits[i] == (lhs.bits[i] & rhs.bits[i]))) {
        i++;
    }
    if (i != rhs.size) {
        return 0;
    }
    while ((i < lhs.size) && (lhs.bits[i] == 0)) {
        i++;
    }
    return (i == lhs.size ? 1 : 0);
}

int isEqualIntSet(intSet lhs, intSet rhs) {
    int i = 0, upb = minimum(lhs.size, rhs.size);
    while ((i < upb) && (lhs.bits[i] == rhs.bits[i])) {
        i++;
    }
    if (i != upb) {
        return 0;
    }
    while ((i < lhs.size) && (lhs.bits[i] == 0)) {
        i++;
    }
    if (i < lhs.size) {
        return 0;
    }
    while ((i < rhs.size) && (rhs.bits[i] == 0)) {
        i++;
    }
    return (i >= rhs.size ? 1 : 0);
}

int isDisjointIntSet(intSet lhs, intSet rhs) {
    int i = 0, upb = minimum(lhs.size, rhs.size);
    while ((i < upb) && ((lhs.bits[i] & rhs.bits[i]) == 0)) {
        i++;
    }
    return (i >= upb ? 1 : 0);
}

unsigned int chooseFromIntSet(intSet s) {
    unsigned int i = 0, x, val = 0;
    while ((i < s.size) && (s.bits[i] == 0)) {
        val += BITS_UINT;
        i++;
    }
    if (i == s.size) {
        fprintf(stderr, "Fatal error in chooseFromIntSet(s): s is an empty set\n");
        exit(EXIT_FAILURE);
    }
    x = s.bits[i];
    while (x%2 == 0) {
        val++;
        x /= 2;
    }
    return val;
}

void fprintIntSet(FILE *f, intSet s) {
    int i, comma = 0;
    fprintf(f, "{");
    for (i=0; i < s.size; i++) {
        unsigned int x = s.bits[i], n = i*BITS_UINT;
        while (x) {
            if (x%2) {
                if (comma) {
                    fprintf(f, ",");
                }
                fprintf(f, "%d", n);
                comma = 1;
            }
            x /= 2;
            n++;
        }
    }
    fprintf(f, "}");
}

void fprintlnIntSet(FILE *f, intSet s) {
    fprintIntSet(f, s);
    fprintf(f, "\n");
}

void printIntSet(intSet s) {
    fprintIntSet(stdout, s);
}

void printlnIntSet(intSet s) {
    fprintlnIntSet(stdout, s);
}

intSet readIntSetFromFile(FILE *f) {
    intSet s;
    unsigned int value;
    s = makeEmptyIntSet();
    expectCharacter(f, '{');
    while (fscanf(f, "%u", &value)) {
        insertIntSet(value, &s);
        if (peekChar(f) == ',') {
            fgetc(f);
        }
    }
    expectCharacter(f, '}');
    return s;
}
