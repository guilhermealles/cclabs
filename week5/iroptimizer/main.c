#include <stdio.h>
#include <stdlib.h>
#include "quadruple.h"
#include "misc.h"

extern int yyparse();
extern void initLexer(char *fnm);
extern void finalizeLexer();

typedef struct stringPair {
  char *key, *str;
} stringPair;

static stringPair *table = NULL;
static int tableSize=0;     /* number of pairs in the pair table */
static int allocatedSize=0; /* allocated number of table entries */

static void resizeStringTable() {
  allocatedSize = (allocatedSize == 0 ? 1 : 2*allocatedSize);
  table = safeRealloc(table, allocatedSize*sizeof(stringPair));
}

static int lookupInStringTable(char *key) {
  int i;
  for (i=0; i < tableSize; i++) {
    if (areEqualStrings(table[i].key, key)) {
      return i;
    }
  }
  return -1;  /* not found */
}

static void insertStringPair(char *key, char *str) {
  int idx = lookupInStringTable(key);
  if (idx != -1) {
    free(table[idx].str);
    table[idx].str = stringDuplicate(str);
    return;
  }
  if (tableSize == allocatedSize) {
    resizeStringTable();
  }
  table[tableSize].key = stringDuplicate(key);
  table[tableSize].str = stringDuplicate(str);
  tableSize++;
}

static void removeStringPairs(char *key) {
  /* remove any pair (x,y) where x==key or y==key from table */
  int i, idx;
  for (i=idx=0; i < tableSize; i++) {
    if (areEqualStrings(table[i].key, key) || 
        areEqualStrings(table[i].str, key)) {
      free(table[i].key);
      free(table[i].str);
    } else {
      table[idx++] = table[i];
    }
  }
  tableSize = idx;
}

static void deallocateTable() {
  int i;
  for (i=0; i < tableSize; i++) {
    free(table[i].key);
    free(table[i].str);
  }
  free(table);
  table = NULL;
  tableSize = 0;
  allocatedSize = 0;
}


/********************************************************************/

void processQuadruple(quadruple quad) {
  /* PLACE HERE YOUR OWN CODE */
  /* ........................ */

  fprintfQuadruple(stdout, quad);
  fprintf(stdout, "\n");
  freeQuadruple(quad);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    abortMessage("Usage: %s <program.ir>", argv[0]);
  }

  initLexer(argv[1]);
  yyparse();
  finalizeLexer();
  deallocateTable();

  return EXIT_SUCCESS;
}
