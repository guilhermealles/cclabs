%options "thread-safe abort generate-lexer-wrapper generate-symbol-table";
%datatype "struct data *", "data.h";
%start parser, rule;
%lexical lexer;

rule :
	'a'+
;

{
#include <stdio.h>
#include <stdlib.h>

int lexer(struct LLthis *LLthis) {
	return LLdata->string[LLdata->index++];
}

void LLmessage(struct LLthis *LLthis, int LLtoken) {
	switch (LLtoken) {
		case LL_MISSINGEOF:
			fprintf(stderr, "Expected %s, found %s.\n", 
				LLgetSymbol(EOFILE), LLgetSymbol(LLsymb));
			break;
		case LL_DELETE:
			fprintf(stderr, "Unexpected %s.\n",
				LLgetSymbol(LLsymb));
			break;
		default:
			fprintf(stderr, "Expected %s, found %s.\n",
				LLgetSymbol(LLtoken), LLgetSymbol(LLsymb));
			break;
	}

	if (!LLdata->dontStop)
		LLabort(LLthis, 1);
}

int main(int argc, char *argv[]) {
	struct data data;
	int i;

	for (i = 1; i < argc; i++) {
		data.string = argv[i];
		data.index = 0;
		/* Don't stop for odd numbered arguments. */
		data.dontStop = i & 1;
		if (parser(&data) == 1) {
			printf("Failed at argument %i\n", i);
			exit(EXIT_FAILURE);
		}
	}
	exit(EXIT_SUCCESS);
}

}
