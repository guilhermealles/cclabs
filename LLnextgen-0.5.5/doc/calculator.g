%start calculator, input;
%label NUM, "number";
%options "generate-lexer-wrapper generate-llmessage generate-symbol-table";
%lexical lexer;

{
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

static int value;

enum states {
	START,
	NUMBER
};

int lexer(void) {
	enum states state = START;
	int c;
	
	value = 0;
	while ((c = getchar()) != EOF) {
		switch (state) {
			case START:
				if (isspace(c) && c != '\n') {
					/* Skip white space, except for newlines. */
					continue;
				}	else if (isdigit(c)) {
					/* Digits mean a number! */
					state = NUMBER;
					value = c - '0';
					break;
				}
				/* Simply return all other characters and let the
				   parser error handling sort it out if necessary. */
				return c;
			case NUMBER:
				/* Read all digits and push back the non-digit, so
				   we can reread that the next time. */
				if (!isdigit(c)) {
					ungetc(c, stdin);
					return NUM;
				}
				value = value * 10 + (c - '0');
				break;
		}
	}
	/* We're done. */
	return EOFILE;
}

/* Simple main routine to fire up the calculator. */
int main(int argc, char *argv[]) {
	printf("LLnextgen integer-calculator example. Press ^C or ^D to end.\n");
	calculator();
	return 0;
}

/* Define the operator priorities. A table would have been possible
   as well, but this is just as clear and requires less memory. */
int getPriority(int operator) {
	switch(operator) {
		case '-':
		case '+':
			return 0;
		case '/':
		case '%':
			return 1;
		case '*':
			return 2;
		case '^':
			return 3;
	}
	/* This should never happen. */
	abort();
}

}

input :
	'\n' *        /* Empty lines should be skipped. */
	[
		expression(0)
		{
			printf("Answer: %d\n", expression);
		}
		'\n' +    /* Empty lines should be skipped. */
	] *
;

expression<int>(int priority) :
	/* Expressions are factors (numbers, negated expressions and
	   expressions between parentheses) followed by operators,
	   followed by expressions with higher priority. */

	factor<LLretval>
		/* By renaming the return value of expression to LLretval, we
		   immediately set the return value of this rule. */
	[
		%while (getPriority(LLsymb) >= priority)
		/* The %while directive says to keep accumulating operators
		   as long as they have equal or higher priority. */

		'-'
		expression<intermediate>(getPriority('-') + 1)
		/* The getPriority() + 1 means that '-' is left associative.
		   If it needs to be right associative, this needs to be
		   getPriority().
		   Also note the explicit use of '-' instead of LLsymb. This
		   is necessary as LLsymb has changed after matching '-'. */
		{
			LLretval -= intermediate;
		}
	|
		'+'
		expression<intermediate>(getPriority('+') + 1)
		{
			LLretval += intermediate;
		}
	|
		'*'
		expression<intermediate>(getPriority('*') + 1)
		{
			LLretval *= intermediate;
		}
	|
		'/'
		expression<intermediate>(getPriority('/') + 1)
		{
			LLretval /= intermediate;
		}
	|
		'%'
		expression<intermediate>(getPriority('%') + 1)
		{
			LLretval %= intermediate;
		}
	|
		'^'
		expression<intermediate>(getPriority('^') + 1)
		{
			LLretval = (int) pow(LLretval, intermediate);
		}
	] * /* Note: an expression can also be just a number or parenthesised
	       expression, so there can also be 0 operators. Hence the *. */
;

factor<int> :
	'('
	expression<LLretval>(0)
	')'
|
	'-' expression(1)
	{
		LLretval = - expression;
	}
|
	NUM
	{
		LLretval = value; /* value is set by the lexical analyser. */
	}
;
