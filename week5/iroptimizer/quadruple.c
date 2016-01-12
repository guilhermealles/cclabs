#include <stdio.h>
#include <stdlib.h>
#include "quadruple.h"
#include "misc.h"

quadruple makeQuadruple(char *lhs, operator op, char *op1, char *op2) {
  quadruple q;
  q.lhs = stringDuplicate(lhs);
  q.operation = op;
  q.operand1 = stringDuplicate(op1);
  q.operand2 = stringDuplicate(op2);
  return q;
}

void freeQuadruple(quadruple q) {
  free(q.lhs);
  free(q.operand1);
  free(q.operand2);
}

void fprintfQuadruple(FILE *f, quadruple q) {
  fprintf(f, "%s=", q.lhs);
  fprintf(f, "%s", q.operand1);
  switch (q.operation) {
  case ASSIGNMENT: break;
  case PLUSOP    : fprintf(f, "+"); break;
  case MINUSOP   : fprintf(f, "-"); break;
  case TIMESOP   : fprintf(f, "*"); break;
  case DIVOP     : fprintf(f, "/"); break;
  }
  if (q.operation != ASSIGNMENT) {
    fprintf(f, "%s", q.operand2);
  }
  fprintf(f, ";");
}

