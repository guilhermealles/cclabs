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

#include <string.h>

#include "generate.h"
#include "option.h"
#include "io.h"
#include "globals.h"
const char *userDataTypeTS = "void *", *userDataTypeHeaderTS = NULL;
static const char *lexerWrapperHeaderTS = "int LLlexerWrapper(LLthisType *LLthis) {\n";
static const char *llmessageHeaderTS = "void LLmessage(struct LLthis *LLthis, int LLtoken) {\n";

/** Write the header file for thread-safe parsers.
	@param output The @a File to write to.
*/
void generateHeaderFileTS(File *output) {
	size_t setSize;

	generateHeaderFileTop(output);

	tfprintf(output, "#define %s_THREAD_SAFE\n", prefix);

	/* Define the struct for thread-safe parsers. */
	setSize = ((condensedNumber + sizeof(int) * 8 - 1) / (sizeof(int)*8)) * sizeof(int);
	if (setSize == 0)
		setSize = 1; /* ANSI C doesn't allow empty arrays. */

	if (userDataTypeHeaderTS != NULL) {
		size_t length = strlen(userDataTypeHeaderTS);
		if (userDataTypeHeaderTS[0] == '<' && length > 0 && userDataTypeHeaderTS[length - 1] == '>') {
			tfprintf(output, "#include %s\n", userDataTypeHeaderTS);
		} else {
			tfprintf(output, "#include \"%s\"\n", userDataTypeHeaderTS);
		}
	}

	if (option.abort)
		tfputs(output, "#include <setjmp.h>\n");

	tfprintf(output, "struct %sthis {\n"
		"\tint LLscnt_[%d];\n"
		"\tint LLtcnt_[%d];\n"
		"\tint LLcsymb_;\n"
		"\tint LLsymb_;\n", prefix, numberOfSets + 1, condensedNumber);
	if (!option.noLLreissue)
		tfputs(output, "\tint LLreissue_;\n");

	if (option.abort)
		tfputs(output, "\tjmp_buf LLjmp_buf_;\n");
	tfprintf(output, "\t%s LLdata_;\n};\n", userDataTypeTS);

	tfprintf(output, "#define %ssymb (LLthis->LLsymb_)\n", prefix);
	tfprintf(output, "#define %sdata (LLthis->LLdata_)\n", prefix);
	if (!option.noLLreissue)
		tfprintf(output, "#define %sreissue (LLthis->LLreissue_)\n", prefix);

	/* LLgen does not declare the parsing routines in the header file,
	   but we should. This makes tidy programming so much easer. */
	if (!option.noPrototypesHeader) {
		generateParserDeclarations(output);
		if (option.generateSymbolTable)
			tfprintf(output, "const char *%sgetSymbol(int);\n", prefix);
		if (option.abort)
			tfprintf(output, "void %sabort(struct %sthis *, int);\n", prefix, prefix);
	}

	/* Include guard. */
	tfprintf(output, "#endif\n");
}

/** Write the parameters of a thread-safe rule.
	@param output The @a File to write to.
	@param nonTerminal The rule to write the parameters for.
*/
void generateNonTerminalParametersTS(File *output, NonTerminal *nonTerminal) {
	if (nonTerminal->parameters != NULL) {
		tfputs(output, "(LLthisType *LLthis");
		if (nonTerminal->argCount > 0)
			tfputc(output, ',');
		tfputc(output, '\n');
		outputCode(output, nonTerminal->parameters, false);
		tfputs(output, ") ");
	} else {
		tfprintf(output, "(LLthisType *LLthis) ");
	}
}

/** Write the prototypes for all the rules (thread-safe version).
	@param output The @a File to write to.
*/
void generatePrototypesTS(File *output) {
	int i;
	Declaration *declaration;

	/* Note: unfortunately this can't be done with walkRules, because
		generateNonTerminalDeclaration needs the file to output to */
	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == NONTERMINAL)
			generateNonTerminalDeclaration(output, declaration->uNonTerminal);
	}
}


/** Write the set of declarations and #define's common to all thread-safe .c files.
	@param output The @a File to write to.
	@param storageType The storage type ('static', 'extern' or nothing) for
		the variables.

	Most #define's are simply there to allow use of LL<name> instead of
	<prefix><name>. In the case that %prefix isn't used, they are omitted.
	Further, #define's are generated for the number of tokens and sets, and
	some utility macro's.
*/
static void generateDefaultDeclarationsTS(File *output) {
	size_t setSize;
	if (prefixDirective != NULL) {
		generateDefaultPrefixTranslations(output);
		tfprintf(output, "#define LL_THREAD_SAFE\n");
	}

	tfprintf(output, "#define LLthisType struct %sthis\n", prefix);
	tfputs(output, "#define LLscnt (LLthis->LLscnt_)\n");
	tfputs(output, "#define LLtcnt (LLthis->LLtcnt_)\n");
	tfputs(output, "#define LLcsymb (LLthis->LLcsymb_)\n");

	/* Define the number of tokens and the sets */
	tfprintf(output, "#define LL_NTERMINALS %d\n", condensedNumber);
	/* The number of sets is the number bytes in a set. Because we start from
	   int's, we round up the needed number of bytes to a multiple of the size
	   of an int. This makes it easier to generate the sets. */
	setSize = ((condensedNumber + sizeof(int) * 8 - 1) / (sizeof(int)*8)) * sizeof(int);
	tfprintf(output, "#define LL_NSETS %d\n#define LL_SSETS %u\n", numberOfSets, (unsigned int) setSize);
	/* Define LLinset macro. Note that any decent compiler will
	   replace the division with a shift */
	tfprintf(output, "#define LLinset(LLx) (LLsets[LLx*LL_SSETS + (LLcsymb/8)] & (1<<(LLcsymb & 7)))\n");
	tfputs(output, "#define LL_SCANDONE(LLx) if (LLsymb != LLx) LLerror(LLx);\n");
}

/** Write code to define all internal variables and utility functions for thread safe parsers.
	@param output The @a File to write to.
	@param headerName The name of the header file to include.

	These variables and functions end up in the .c file, or in the case of
	LLgen style outputs in the Lpars.c file.
*/
void generateDefaultGlobalDeclarationsTS(File *output, const char *headerName) {
	const char *lexer, *lexerInternal = NULL; /* The = NULL is there to shut up the compiler */
	const char *storageType = option.LLgenStyleOutputs ? STORAGE_GLOBAL : STORAGE_STATIC;

	lexer = lexicalDirective ? lexicalDirective->token[0]->text : "yylex";
	if (option.generateLexerWrapper) {
		lexerInternal = lexer;
		lexer = "LLlexerWrapper";
	}

	generateDefaultDeclarationsTS(output);

	tfputs(output, "#include <string.h>\n");

	generateConversionTables(output, storageType);

	/* Generate the LLfirst routine, but only if a %first directive is used. */
	if (hasFirst) {
		tfprintf(output,
			"%sint LLfirst(int LLtoken, int LLset) {\n"
				"\tint LLctoken;\n"
				"\treturn (LLtoken >= -1 && LLtoken < %d && (LLctoken = LLindex[LLtoken+1]) >= 0 &&\n"
					"\t\t(LLsets[LLset*LL_SSETS + (LLctoken/8)] & (1<<(LLctoken & 7))));\n"
			"}\n", storageType, maxTokenNumber);
	}

	/* Generate the symbol table and LLgetSymbol routine, but only if a the
	   --generateSymbolTable option is used. */
	if (option.generateSymbolTable)
		generateSymbolTable(output);

	if (headerName != NULL) {
		/* Include our header file */
		tfprintf(output, "#include \"%s\"\n", headerName);
	}

	/* Declare the lexer */
	tfprintf(output, "int %s(struct %sthis *);\n", lexerInternal != NULL ? lexerInternal : lexer, prefix);

	if (option.generateLexerWrapper) {
		tfputs(output, "static ");
		tfputs(output, lexerWrapperHeaderTS);
		tfprintf(output, lexerWrapperString, "-2 /* LL_NEW_TOKEN */", lexerInternal, "LLthis", "-2 /* LL_NEW_TOKEN */");
	}

	/* As we generate our standard LLmessage as a static routine, we need to
	   declare it static here as well to prevent compiler warnings. */
	if (option.generateLLmessage)
		tfputs(output, "static ");
	/* Declare LLmessage (or actually <%prefix>message, because of the defines) */
	tfprintf(output, "void LLmessage(struct %sthis *, int);\n", prefix);

	/* Define LLread. For correcting error recovery, it can be really simple.
	   As a good compiler will inline this anyway, we don't have to bother
	   with a #define. */
	tfprintf(output, "%svoid LLread(LLthisType *LLthis) { LLcsymb = LLindex[(LLsymb = %s(LLthis)) + 1]; }\n", storageType, lexer);

	/* Define the error handling routines LLskip and LLerror
	   Note: LLskip is spread over two tfprintf statements because otherwise
	   the format string would exceed the maximum length for a string constant.
	*/
	tfprintf(output,
		"%sint LLskip(LLthisType *LLthis) {\n"
			"\tint LL_i;\n"
			"\tif (LLcsymb >= 0) {\n"
				"\t\tif (LLtcnt[LLcsymb] != 0) return 0;\n"
				"\t\tfor (LL_i = 0; LL_i < LL_NSETS; LL_i++)\n"
					"\t\t\tif (LLscnt[LL_i] != 0 && LLinset(LL_i))\n"
						"\t\t\t\treturn 0;\n"
			"\t}\n\n", storageType);
	tfprintf(output,
			"\tfor (;;) {\n"
				"\t\tLLmessage(LLthis, %d /* LL_DELETE */);\n"
				"\t\twhile ((LLcsymb = LLindex[(LLsymb = %s(LLthis)) + 1]) < 0) LLmessage(LLthis, %d /* LL_DELETE */);\n"
				"\t\tif (LLtcnt[LLcsymb] != 0)\n"
					"\t\t\treturn 1;\n"
				"\t\tfor (LL_i = 0; LL_i < LL_NSETS; LL_i++)\n"
					"\t\t\tif (LLscnt[LL_i] != 0 && LLinset(LL_i))\n"
						"\t\t\t\treturn 1;\n"
		"\t}\n}\n", option.noEOFZero ? -2 : 0, lexer, option.noEOFZero ? -2 : 0);

	tfprintf(output,
		"%svoid LLerror(LLthisType *LLthis, int LLtoken) {\n"
			"\tif (LLtoken == 256 /* EOFILE */) {\n"
				"\t\tLLmessage(LLthis, -1 /* LL_MISSINGEOF */);\n"
				"\t\twhile (LLindex[(LLsymb = %s(LLthis)) + 1] != 0) /*NOTHING*/ ;\n"
				"\t\treturn;\n"
			"\t}\n"
			"\tLLtcnt[LLindex[LLtoken + 1]]++;\n"
			"\tLLskip(LLthis);\n"
			"\tLLtcnt[LLindex[LLtoken + 1]]--;\n"
			"\tif (LLsymb != LLtoken) {%s LLmessage(LLthis, LLtoken); }\n"
		"}\n", storageType, lexer, option.noLLreissue ? "" : " LLreissue = LLsymb;");

	/* Generate the LLabort routine, but only if a the --abort option is used. */
	if (option.abort) {
		tfputs(output,
			"void LLabort(LLthisType *LLthis, int LLretval) {\n"
				"\tlongjmp(LLthis->LLjmp_buf_, LLretval);\n"
			"}\n");
	}

	/* Add macro definitions that will keep us from having to change code
	   generation in the general case. */
	tfputs(output, "#define LLread() LLread(LLthis)\n");
	tfputs(output, "#define LLskip() LLskip(LLthis)\n");
	tfputs(output, "#define LLerror(LLx) LLerror(LLthis, LLx)\n");

	if (option.generateLLmessage) {
		tfputs(output, "#include <stdio.h>\nstatic ");
		tfputs(output, llmessageHeaderTS);
		tfputs(output, llmessageString[0]);
		tfputs(output, llmessageString[1]);
	}
}


/*===== Code to generate parser =====*/
/** Write the code for a thread safe parser.
	@param output The @a File to write to.
	@param directive The %start directive to write code for.
*/
static void generateParserTS(File *output, Directive *directive) {
	NonTerminal *nonTerminal;

	nonTerminal = ((Declaration *) lookup(globalScope, directive->token[1]->text))->uNonTerminal;

	/* NOTE: as we cannot move this function to before the header file inclusion
	   (thread safe parsers will likely have some user defined type as
	   argument), symbols created by the user might collide with ours (although
	   not for LLgen-style output). Therefore we prefix our vars so we can
	   consider it a user problem (don't say we didn't warn you in the manual). */

#ifdef DEBUG
	/* To prevent warnings about implicit declarations, we add these includes */
	tfputs(output, "#include <stdio.h>\n#include <stdlib.h>\n");
#endif

	generateParserHeader(output, directive);
	tfputs(output, " {\n");

	tfputs(output, "\tLLthisType LLthis;\n");
	if (option.abort)
		tfputs(output, "\tint LLsetjmpRetval;\n");
	/* Clear the contains set tracking variables. */
	tfputs(output, "\tmemset(LLthis.LLscnt_, 0, LL_NSETS * sizeof(int));\n");
	tfputs(output, "\tmemset(LLthis.LLtcnt_, 0, LL_NTERMINALS * sizeof(int));\n");
	/* EOFILE is always in the contains set: */
	tfputs(output, "\tLLthis.LLtcnt_[0]++;\n");
	tfputs(output, "\tLLthis.LLdata_ = LLuserData;\n");
	tfputs(output, "\tLLthis.LLsymb_ = 0;\n");

	if (!option.noLLreissue)
		tfputs(output, "\tLLthis.LLreissue_ = -2 /* LL_NEW_TOKEN */;\n");

	if (option.abort)
		tfputs(output, "\tLLsetjmpRetval = setjmp(LLthis.LLjmp_buf_);\n\tif (LLsetjmpRetval == 0) {\n");
	tfputs(output, "\tLLread(&LLthis);\n");

	if (nonTerminal->term.flags & TERM_MULTIPLE_ALTERNATIVE) {
		/* The lines below do what the call
		   generateSetPushPop(output, nonTerminal->term.contains, DIR_PUSH);
		   normally would do. However, because of the macro's etc. this doesn't
		   work in this specific routine.
		*/
		int size = setFill(nonTerminal->term.contains);
		if (size == 1)
			tfprintf(output, "LLthis.LLtcnt_[%d]++;\n", setFirstToken(nonTerminal->term.contains));
		else if (size > 1)
			tfprintf(output, "LLthis.LLscnt_[%d]++;\n", setFindIndex(nonTerminal->term.contains, false));
	}

	tfputc(output, '\t');
	if (nonTerminal->retvalIdent != NULL)
		tfputs(output, "*LLretval = ");

	tfprintf(output, "%s%d_%s(&LLthis);\n", prefix, nonTerminal->number, directive->token[1]->text);
	if (nonTerminal->term.flags & TERM_NO_FINAL_READ)
		tfputs(output, "\tLLread(&LLthis);\n");
	tfputs(output, "\tif (LLthis.LLcsymb_ != 0) LLerror(&LLthis, 256 /* EOFILE*/);\n");
#ifdef DEBUG
	tfputs(output, "\tLLthis.LLtcnt_[0]--;\n");
	tfputs(output, "{ int LL_i; for(LL_i = 0; LL_i < LL_NTERMINALS; LL_i++) if (LLthis.LLtcnt_[LL_i] != 0) { printf(\"Counts are not all zero!!!!\\n\"); exit(10); }}\n");
	tfputs(output, "{ int LL_i; for(LL_i = 0; LL_i < LL_NSETS; LL_i++) if (LLthis.LLscnt_[LL_i] != 0) { printf(\"Counts are not all zero!!!!\\n\"); exit(10); }}\n");
#endif
	if (option.abort)
		tfputs(output, "\t}\n");

	if (option.abort)
		tfputs(output, "\treturn LLsetjmpRetval;\n");
	tfputs(output, "}\n\n");
}

/** Write code for directives that can only appear at the end of the .c file (
		thread-safe version).
	@param output The @a File to write to.
*/
void generateDirectiveCodeAtEndTS(File *output) {
	int i;
	Declaration *declaration;

	tfputs(output, "#undef LLerror\n#undef LLread\n");

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == DIRECTIVE && declaration->uDirective->subtype == START_DIRECTIVE)
			generateParserTS(output, declaration->uDirective);
	}
}
