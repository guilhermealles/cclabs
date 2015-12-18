#ifndef NFA_H
#define NFA_H
#include "intset.h"

/* Do not change EPSILON! There are 128 ASCII characters. */
#define EPSILON 128
typedef struct nfa {
    unsigned int nstates;  /* number of states                          */
    unsigned int start;    /* number of thestart state                  */
    intSet final;          /* set of final (accepting) states           */
    intSet **transition;   /* transition: state x char -> set of states */
} nfa;

typedef nfa dfa;

static void *safeMalloc(unsigned int sz);
nfa makeNFA(int nstates);
void freeNFA(nfa n);
nfa readNFA(char *filename);
void saveNFA(char *filename, nfa n);
int alreadyMapped(intSet states);
intSet epsilonClosure(int state, nfa n);
intSet epsilonStarClosure(int state, nfa n);
intSet epsilonStarClosureSet(intSet states, nfa n);
intSet movement(unsigned int state, unsigned int symbol, nfa automaton);
intSet movementSet(intSet states, unsigned int symbol, nfa automaton);
dfa convertNFAtoDFA(nfa n);
intSet addToAllIntSetItems(unsigned int value, intSet set);
void copyTransitions(nfa sourceNfa, unsigned int sourceState, nfa *destNfa, unsigned int completedNfaStates);
void processDfaString(char *string, dfa d);
nfa mergeNFAs(nfa *nfaArray, unsigned int nfaCount);
nfa uniteNFAs(nfa nfa1, nfa nfa2);
nfa concatenateNFAs(nfa nfa1, nfa nfa2);
nfa kleeneClosureNFA(nfa nfa);
nfa optionalOperationNFA(nfa nfa);
nfa positiveClosureNFA(nfa nfa);
nfa regexpToNFA(char* regexp);

#endif
