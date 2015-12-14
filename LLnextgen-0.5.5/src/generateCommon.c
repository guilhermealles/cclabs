/* Copyright (C) 2005-2008,2011 G.P. Halkes
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
#include <ctype.h>
#include <stdlib.h>

#include "generate.h"
#include "nonRuleAnalysis.h"
#include "bool.h"
#include "list.h"
#include "option.h"
#include "globals.h"
#include "io.h"

/** Construct the name of an output file, from the name of an input file.
	@param originalName The name of the input file.
	@param extension The extension of the output file.

	The file constructed will be the originalName with a trailing '.g'
	removed, after which the extension is added.
*/
char *createNameWithExtension(const char *originalName, const char *extension) {
	bool addExtension;
	char *fullName;
	size_t length, extensionLength;

	extensionLength = strlen(extension) + 1;

	length = strlen(originalName);

	/* Check for '.g' */
	addExtension = !(length >= 2 && originalName[length - 1] == 'g' && originalName[length - 2] == '.');
	length += extensionLength;

	if (!addExtension)
		length -= 2;

	/* Add 1 for the 0 byte */
	fullName = (char *) safeMalloc(length + 1, "openOutputs");
	strcpy(fullName, originalName);

	if (addExtension)
		fullName[length - extensionLength] = '.';

	strcpy(fullName + length - extensionLength + 1, extension);
	return fullName;
}

/** Write a code fragment that was read from the input to the output.
	@param output The @a File to write to.
	@param code The code to write.
	@param keepMarkers Boolean to indicate whether to keep the parentheses or
		braces that delimited the input.
*/
void outputCode(File *output, CCode *code, bool keepMarkers) {
	tfprintDirective(output, code);

	if (keepMarkers) {
		tfputs(output, code->text);
	} else {
		size_t length;
		length = strlen(code->text);

		tfputsn(output, code->text + 1, length - 2);
	}
	tfputc(output, '\n');

	tfprintDirective(output, NULL);
}

/*================== Code to generate the header file ========================*/
/** Write the #define's for the tokens.
	@param output The @a File to write to.
*/
static void generateTokenDeclarations(File *output) {
	int i;
	Declaration *declaration;
	Directive *directive;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == DIRECTIVE && declaration->uDirective->subtype == TOKEN_DIRECTIVE) {
			directive = declaration->uDirective;
			tfprintf(output, "#define %s %d\n", directive->token[0]->text, directive->uNumber);
		}
	}
}

void generateParserHeader(File *output, Directive *directive) {
	Declaration *startRule;

	tfprintf(output, "%s %s(", option.abort ? "int" : "void", directive->token[0]->text);

	startRule = (Declaration *) lookup(globalScope, directive->token[1]->text);
	ASSERT(startRule != NULL);

	if (option.threadSafe) {
		tfprintf(output, "%s LLuserData", userDataTypeTS);
		if (startRule->uNonTerminal->retvalIdent != NULL)
			tfprintf(output, ", %s *LLretval", startRule->uNonTerminal->retvalIdent->text);
	} else if (startRule->uNonTerminal->retvalIdent != NULL) {
		tfprintf(output, "%s *LLretval", startRule->uNonTerminal->retvalIdent->text);
	} else {
		tfputs(output, "void");
	}

	tfputs(output, ")");
}

/** Write a prototype for each parser.
	@param output The @a File to write to.
*/
void generateParserDeclarations(File *output) {
	int i;
	Declaration *declaration;
	Directive *directive;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == DIRECTIVE && declaration->uDirective->subtype == START_DIRECTIVE) {
			directive = declaration->uDirective;
			generateParserHeader(output, directive);
			tfputs(output, ";\n");
		}
	}
}

/** Write the top of a header file.
	@param output The @a File to write to.
*/
void generateHeaderFileTop(File *output) {
	/* Include guard. */
	tfprintf(output, "#ifndef  __LLNEXTGEN_%s_H__\n#define __LLNEXTGEN_%s_H__\n", prefix, prefix);

	tfputs(output, "#ifndef LL_NOTOKENS\n");
	tfputs(output, "#define EOFILE 256\n");
	generateTokenDeclarations(output);
	tfputs(output, "#endif\n");
	/* Add some defines in the way that the prefix directive has effect. */
	tfprintf(output, "#define %s_MAXTOKNO %d\n", prefix, maxTokenNumber - 1);
	tfprintf(output, "#define %s_MISSINGEOF (-1)\n", prefix);
	tfprintf(output, "#define %s_DELETE (%d)\n", prefix, option.noEOFZero ? -2 : 0);
	tfprintf(output, "#define %s_VERSION " VERSION_HEX "\n", prefix);
	if (!option.noLLreissue)
		tfprintf(output, "#define %s_NEW_TOKEN (-2)\n", prefix);
}
/** Write the header file.
	@param output The @a File to write to.
*/
void generateHeaderFile(File *output) {
	generateHeaderFileTop(output);

	/* LLgen does not declare the parsing routines in the header file,
	   but we should. This makes tidy programming so much easer. */
	if (!option.noPrototypesHeader) {
		generateParserDeclarations(output);
		if (option.generateSymbolTable)
			tfprintf(output, "const char *%sgetSymbol(int);\n", prefix);
		if (option.abort)
			tfprintf(output, "void %sabort(int);\n", prefix);
		tfprintf(output, "extern int %ssymb;\n", prefix);
		if (!option.noLLreissue && !option.generateLexerWrapper)
			tfprintf(output, "extern int %sreissue;\n", prefix);
	}

	/* Include guard. */
	tfprintf(output, "#endif\n");
}

/*=============== Code to generate the first part of a C file ================*/
/** Write the set of translations from LL to <prefix> variants.
	@param output The @a File to write to.
*/
void generateDefaultPrefixTranslations(File *output) {
	/* Add some defines so that the prefix directive has effect. Note
	   that only externally visible symbols need to be renamed. */
	tfprintf(output, "#define LLsymb %ssymb\n", prefix);
	tfprintf(output, "#define LLmessage %smessage\n", prefix);
	if (!option.noLLreissue && !option.generateLexerWrapper)
		tfprintf(output, "#define LLreissue %sreissue\n", prefix);
	if (option.abort)
		tfprintf(output, "#define LLabort %sabort\n", prefix);
	if (option.generateSymbolTable)
		tfprintf(output, "#define LLgetSymbol %sgetSymbol\n", prefix);
	/* We want these available under their LL names in the C file regardless */
	tfputs(output, "#define LL_MISSINGEOF (-1)\n");
	tfprintf(output, "#define LL_DELETE (%d)\n", option.noEOFZero ? -2 : 0);
	tfputs(output, "#define LL_VERSION " VERSION_HEX "\n");
	if (!option.noLLreissue)
		tfputs(output, "#define LL_NEW_TOKEN (-2)\n");
}

/** Write the set of declarations and #define's common to all .c files.
	@param output The @a File to write to.
	@param storageType The storage type ('static', 'extern' or nothing) for
		the variables.

	Most #define's are simply there to allow use of LL<name> instead of
	<prefix><name>. In the case that %prefix isn't used, they are omitted.
	Further, #define's are generated for the number of tokens and sets, and
	some utility macro's. The variables LLscnt, LLtcnt and LLcsymb are also
	declared.
*/
static void generateDefaultDeclarations(File *output, const char *storageType) {
	size_t setSize;
	if (prefixDirective != NULL) {
		generateDefaultPrefixTranslations(output);
		if (option.LLgenStyleOutputs) {
			/* In the case of LLgen style outputs, we need to do renaming of
			   these as well, as these symbols are not static as they are for
			   LLnextgen style outputs. */
			tfprintf(output, "#define LLread %sread\n", prefix);
			tfprintf(output, "#define LLskip %sskip\n", prefix);
			tfprintf(output, "#define LLfirst %sfirst\n", prefix);
			tfprintf(output, "#define LLerror %serror\n", prefix);
			tfprintf(output, "#define LLsets %ssets\n", prefix);
			tfprintf(output, "#define LLindex %sindex\n", prefix);
			tfprintf(output, "#define LLcsymb %scsymb\n", prefix);
			tfprintf(output, "#define LLscnt %sscnt\n", prefix);
			tfprintf(output, "#define LLtcnt %stcnt\n", prefix);
		}
	}

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

	/* For reentrant scanners, we don't use the global variables as arrays,
	   but as pointers to stack arrays. This saves us global memory space and
	   memory copying. */
	if (!option.reentrant)
		/* Add one the the number of sets, to make sure we don't end up with an
		   array of size 0. */
		tfprintf(output, "%sint LLscnt[LL_NSETS+1], LLtcnt[LL_NTERMINALS];\n", storageType);
	else
		tfprintf(output, "%sint *LLscnt, *LLtcnt;\n", storageType);

	tfprintf(output, "%sint LLcsymb;\n", storageType);
}

/** Write the header of an LLgen style .c file.
	@param output The @a File to write to.
	@param headerName The name of the header file to include.

	These declarations are only necessary for LLgen style .c files. The
	LLnextgen style .c file includes the definitions, so these declarations
	are superfluous.
*/
void generateDefaultLocalDeclarations(File *output, const char *headerName) {
	/* Include our header file */
	tfprintf(output, "#include \"%s\"\n", headerName);
	generateDefaultDeclarations(output, STORAGE_EXTERN);

	/* LLsets and LLindex are defined here different from the rest because
	   they need to be initialised when the storage class is not extern */
	tfputs(output, "extern const char LLsets[];\n");
	tfputs(output, "extern const int LLindex[];\n");
	/* LLsymb is defined here because it needs to be extern or global, and
	   never static (not even for LLnextgen outputs). This is because a
	   user can supply a LLmessage routine outside of the generated files and
	   in LLmessage the user needs LLsymb. */
	tfputs(output, "extern int LLsymb;\n");
	/* For LLreissue a similar reasoning holds as for LLsymb, only based on
	   the lexer(-wrapper). */
	if (!option.noLLreissue && !option.generateLexerWrapper)
		tfprintf(output, "extern int LLreissue;\n");

	/* These functions are here because we don't need these headers at all
	   when generating LLnextgen style outputs. */
	tfputs(output, "void LLread(void);\n");
	tfputs(output, "int LLskip(void);\n");
	if (hasFirst)
		tfputs(output, "int LLfirst(int LLtoken, int LLset);\n");
	tfputs(output, "void LLerror(int LLtoken);\n");
}

/** Write code for a directive.
	@param output The @a File to write to.
	@param directive The directive to write code for.

	At this point the only directive that needs code written here is the
	%first directive. The other directives apart from %start are integrated
	in the code. The %start directive is not generated with this routine, as
	for LLgen we need to generate the parsers only in Lpars.c and not for each
	.c file.
*/
static void generateDirectiveDeclaration(File *output, Directive *directive) {
	switch (directive->subtype) {
		case START_DIRECTIVE:
		case TOKEN_DIRECTIVE:
 		case LEXICAL_DIRECTIVE:
		case PREFIX_DIRECTIVE:
		case LABEL_DIRECTIVE:
		case DATATYPE_DIRECTIVE:
			/* Generated elsewhere */
			break;
		case FIRST_DIRECTIVE: {
			Declaration *declaration;
			NonTerminal *nonTerminal;
			int fill;

			declaration = (Declaration *) lookup(globalScope, directive->token[1]->text);
			ASSERT(declaration != NULL && declaration->subtype == NONTERMINAL);
			nonTerminal = declaration->uNonTerminal;

			fill = setFill(nonTerminal->term.first);
			if (fill > 1) {
				tfprintf(output, "#define %s(LLx) LLfirst(LLx, %d)\n", directive->token[0]->text, setFindIndex(nonTerminal->term.first, false));
			} else if (fill == 1) {
				int token = setFirstToken(nonTerminal->term.first);
				if (condensedToTerminal[token].flags & CONDENSED_ISTOKEN)
					token = condensedToTerminal[token].uToken->uNumber;
				else
					token = condensedToTerminal[token].uLiteral;
				tfprintf(output, "#define %s(LLx) (LLx == %d)\n", directive->token[0]->text, token);
			} else {
				tfprintf(output, "#define %s(LLx) 0\n", directive->token[0]->text);
			}
			break;
		}
		case ONERROR_DIRECTIVE:
			/* NOTE: not supported yet */
			break;
		default:
			PANIC();
	}

}

/** Write code for all directives.
	@param output The @a File to write to.
*/
void generateDirectiveDeclarations(File *output) {
	int i;
	Declaration *declaration;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == DIRECTIVE)
			generateDirectiveDeclaration(output, declaration->uDirective);
	}
}

/** Write the name (and return type) of a rule.
	@param output The @a File to write to.
	@param nonTerminal The rule to write the name for.
*/
static void generateNonTerminalName(File *output, NonTerminal *nonTerminal) {
	if (!option.LLgenStyleOutputs)
		tfputs(output, "static ");
	tfprintf(output, "%s %s%d_%s", nonTerminal->retvalIdent == NULL ? "void" : nonTerminal->retvalIdent->text, prefix, nonTerminal->number, nonTerminal->token->text);
}

/** Write the parameters of a rule.
	@param output The @a File to write to.
	@param nonTerminal The rule to write the parameters for.
*/
static void generateNonTerminalParameters(File *output, NonTerminal *nonTerminal) {
	if (nonTerminal->parameters != NULL) {
		tfputc(output, '\n');
		outputCode(output, nonTerminal->parameters, true);
	} else {
		tfprintf(output, "(void)");
	}
}

/** Write the prototype for a rule.
	@param output The @a File to write to.
	@param nonTerminal The rule to write the prototype for.
*/
void generateNonTerminalDeclaration(File *output, NonTerminal *nonTerminal) {
	/* Don't generate code for unreachable non-terminals */
	if (!(nonTerminal->flags & NONTERMINAL_REACHABLE))
		return;
	/* Generate declarations for non-terminals. LLgen is minimalistic in the
	   declarations it generates, but there is not really a reason to do so. */
	generateNonTerminalName(output, nonTerminal);
	if (!option.threadSafe)
		generateNonTerminalParameters(output, nonTerminal);
	else
		generateNonTerminalParametersTS(output, nonTerminal);
	tfprintf(output, ";\n");
}

/** Write the prototypes for all the rules.
	@param output The @a File to write to.
*/
void generatePrototypes(File *output) {
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


/*=============== Code to generate the first part of LLnextgen C file or Lpars.c ================*/
static const char *defaultSymbolTable[] = { "\"<EOF>\"", /* For converting -1 */
	/* NOTE: the EOF name for -1 includes the quotes, to make it easier to write
	   the code for overriding the default name. */
	"<EOF>", "<SOH>", "<STX>", "<ETX>", "<EOT>", "<ENQ>", "<ACK>", "<BEL>",
	"<BS>", "<TAB>", "<NL>", "<VT>", "<FF>", "<CR>", "<SO>", "<SI>",
	"<DLE>", "<DC1>", "<DC2>", "<DC3>", "<DC4>", "<NAK>", "<SYN>", "<ETB>",
	"<CAN>", "<EM>", "<SUB>", "<ESC>", "<FS>", "<GS>", "<RS>", "<US>",
	"<SP>", "'!'", "'\\\"'", "'#'", "'$'", "'%'", "'&'", "'\\''",
	"'('", "')'", "'*'", "'+'", "','", "'-'", "'.'", "'/'",
	"'0'", "'1'", "'2'", "'3'", "'4'", "'5'","'6'", "'7'",
	"'8'", "'9'", "':'", "';'", "'<'", "'='", "'>'", "'?'",
	"'@'", "'A'", "'B'", "'C'", "'D'", "'E'", "'F'", "'G'",
	"'H'", "'I'", "'J'", "'K'", "'L'", "'M'", "'N'", "'O'",
	"'P'", "'Q'", "'R'", "'S'", "'T'", "'U'", "'V'", "'W'",
	"'X'", "'Y'", "'Z'", "'['", "'\\\\'", "']'", "'^'", "'_'",
	"'`'", "'a'", "'b'", "'c'", "'d'", "'e'", "'f'", "'g'",
	"'h'", "'i'", "'j'", "'k'", "'l'", "'m'", "'n'", "'o'",
	"'p'", "'q'", "'r'", "'s'", "'t'", "'u'", "'v'", "'w'",
	"'x'", "'y'", "'z'", "'{'", "'|'", "'}'", "'~'", "<DEL>",
	"'\\x80'", "'\\x81'", "'\\x82'", "'\\x83'", "'\\x84'", "'\\x85'", "'\\x86'", "'\\x87'",
	"'\\x88'", "'\\x89'", "'\\x8A'", "'\\x8B'", "'\\x8C'", "'\\x8D'", "'\\x8E'", "'\\x8F'",
	"'\\x90'", "'\\x91'", "'\\x92'", "'\\x93'", "'\\x94'", "'\\x95'", "'\\x96'", "'\\x97'",
	"'\\x98'", "'\\x99'", "'\\x9A'", "'\\x9B'", "'\\x9C'", "'\\x9D'", "'\\x9E'", "'\\x9F'",
	"'\\xA0'", "'\\xA1'", "'\\xA2'", "'\\xA3'", "'\\xA4'", "'\\xA5'", "'\\xA6'", "'\\xA7'",
	"'\\xA8'", "'\\xA9'", "'\\xAA'", "'\\xAB'", "'\\xAC'", "'\\xAD'", "'\\xAE'", "'\\xAF'",
	"'\\xB0'", "'\\xB1'", "'\\xB2'", "'\\xB3'", "'\\xB4'", "'\\xB5'", "'\\xB6'", "'\\xB7'",
	"'\\xB8'", "'\\xB9'", "'\\xBA'", "'\\xBB'", "'\\xBC'", "'\\xBD'", "'\\xBE'", "'\\xBF'",
	"'\\xC0'", "'\\xC1'", "'\\xC2'", "'\\xC3'", "'\\xC4'", "'\\xC5'", "'\\xC6'", "'\\xC7'",
	"'\\xC8'", "'\\xC9'", "'\\xCA'", "'\\xCB'", "'\\xCC'", "'\\xCD'", "'\\xCE'", "'\\xCF'",
	"'\\xD0'", "'\\xD1'", "'\\xD2'", "'\\xD3'", "'\\xD4'", "'\\xD5'", "'\\xD6'", "'\\xD7'",
	"'\\xD8'", "'\\xD9'", "'\\xDA'", "'\\xDB'", "'\\xDC'", "'\\xDD'", "'\\xDE'", "'\\xDF'",
	"'\\xE0'", "'\\xE1'", "'\\xE2'", "'\\xE3'", "'\\xE4'", "'\\xE5'", "'\\xE6'", "'\\xE7'",
	"'\\xE8'", "'\\xE9'", "'\\xEA'", "'\\xEB'", "'\\xEC'", "'\\xED'", "'\\xEE'", "'\\xEF'",
	"'\\xF0'", "'\\xF1'", "'\\xF2'", "'\\xF3'", "'\\xF4'", "'\\xF5'", "'\\xF6'", "'\\xF7'",
	"'\\xF8'", "'\\xF9'", "'\\xFA'", "'\\xFB'", "'\\xFC'", "'\\xFD'", "'\\xFE'", "'\\xFF'" };

/** Header for regualar lexer wrapper. */
static const char *lexerWrapperHeader = "int LLlexerWrapper(void) {\n";

/** The code for the lexer wrapper, with a %s in it as a place holder for the
	real lexer. */
const char *lexerWrapperString =
		"\tif (LLreissue == %s) {\n"
			"\t\treturn %s(%s);\n"
		"\t} else {\n"
			"\t\tint LLretval = LLreissue;\n"
			"\t\tLLreissue = %s;\n"
			"\t\treturn LLretval;\n"
		"\t}\n"
	"}\n";

/** Header for regular llmessage. */
static const char *llmessageHeader = "void LLmessage(int LLtoken) {\n";

/* Note: this string is too long to be one string (at least according to the
   ANSI C standard). That is why it is an array of two strings. */
/** The code for the default LLmessage.

	NOTE: DON'T use printf to write this string. It contains fprintf
	statements with format strings that are not escaped.
*/
const char *llmessageString[2] = {
		"\tswitch (LLtoken) {\n"
			"\t\tcase LL_MISSINGEOF:\n"
				"\t\t\tfprintf(stderr, \"Expected %s, found %s. Skipping.\\n\", LLgetSymbol(EOFILE), LLgetSymbol(LLsymb));\n"
				"\t\t\tbreak;\n", /* Note the comma, to break it into two strings. */
			"\t\tcase LL_DELETE:\n"
				"\t\t\tfprintf(stderr, \"Skipping unexpected %s.\\n\", LLgetSymbol(LLsymb));\n"
				"\t\t\tbreak;\n"
			"\t\tdefault:\n"
				"\t\t\tfprintf(stderr, \"Expected %s, found %s. Inserting.\\n\", LLgetSymbol(LLtoken), LLgetSymbol(LLsymb));\n"
				"\t\t\tbreak;\n"
		"\t}\n"
	"}\n"};

/** Write the conversion tables used in the parser to file.
	@param output The @a File to write to.
	@param storageType The storage type ('static', 'extern' or nothing) for
		the variables.

	These tables can be used both for thread-safe and regular parsers, as no
	changes to them are made.
*/
void generateConversionTables(File *output, const char *storageType) {
	size_t j, k, setSize;
	int i;
	/* Generate the table with token sets. To allow generating code on a
	   machine with for example 4 byte words for a machine with 2 byte words
	   we use char's here instead of int's. */
	setSize = (condensedNumber + sizeof(int) * 8 - 1) / (sizeof(int)*8);
	tfprintf(output, "%sconst char LLsets[] = {\n", storageType);
	for (i = 0; i < numberOfSets; i++) {
		for (j = 0; j < setSize; j++)
			for (k = 0; k < sizeof(int); k++)
				tfprintf(output, "\t'\\x%02X', ", (setList[i].bits[j] >> (k*8)) & 0xff);
		tfputc(output, '\n');
	}
	tfprintf(output, "\t0\n};\n");

	/* Output token number conversion table */
	tfprintf(output, "%sconst int LLindex[] = { 0", storageType);
	for (i = 0; i < maxTokenNumber; i++) {
		if ((i % 8) == 0)
			tfprintf(output, ",\n\t");
		else
			tfprintf(output, ", ");
		tfprintf(output, "%4d", terminalToCondensed[i]);
	}
	tfprintf(output, "};\n");
}

/** Write a symbol table and the LLgetSymbol routine.
	@param output The @a File to write to.
*/
void generateSymbolTable(File *output) {
	Declaration *declaration;
	const char *eofLabel = NULL;
	size_t j;
	int i;

	if (option.noEOFZero)
		defaultSymbolTable[1] = "<NUL>";

	/* use the EOFILE for -1 and also for 0 if !option.noEOFZero */
	declaration = (Declaration *) lookup(globalScope, "EOFILE");
	ASSERT(declaration != NULL && declaration->subtype == DIRECTIVE);
	if (declaration->uDirective->token[1] != NULL) {
		eofLabel = declaration->uDirective->token[1]->text;
		if (!option.noEOFZero)
			literalLabels[0] = declaration->uDirective->token[1];
	}

	/* Write the first part of the symbol table, using the user supplied
	   symbolnames if available. */
	/* NOTE: The EOF names already include the quotes. */
	tfputs(output, "static const char *LLsymbolTable[] = {\n");

	/* Gettext-noop macro definition. Note: we do this AFTER the definition of
	   the variable, so that we avoid any chance of name collisions. */
	if (option.gettext)
		tfprintf(output, "#define %s(LLx) LLx\n", option.gettextMacro);

	if (option.gettext && eofLabel)
		tfprintf(output, "%s(", option.gettextMacro);
	tfputs(output, eofLabel == NULL ? defaultSymbolTable[0] : eofLabel);
	tfputs(output, option.gettext && eofLabel ? "), " : ", ");

	for (i = 0; i < 256; i++) {
		if ((i % 4) == 0)
			tfputc(output, '\n');
		if (literalLabels[i] == NULL) {
			tfprintf(output, "\"%s\", ", defaultSymbolTable[i + 1]);
		} else {
			if (option.gettext)
				tfprintf(output, "%s(", option.gettextMacro);
			tfprintf(output, "%s", literalLabels[i]->text);
			tfputs(output, option.gettext ? "), " : ", ");
		}
	}
	tfputc(output, '\n');
	/* NOTE: The EOF names already include the quotes. */
	if (option.gettext && eofLabel)
		tfprintf(output, "%s(", option.gettextMacro);
	tfputs(output, eofLabel == NULL ? defaultSymbolTable[0] : eofLabel);
	if (option.gettext && eofLabel)
		tfputs(output, ")");

	/* Write the symbol names for the %token defined tokens. */
	for (i = 257; i < maxTokenNumber; i++) {
		Directive *directive;
		tfputs(output, ", ");

		if ((i % 4) == 0)
			tfputc(output, '\n');
		directive = condensedToTerminal[terminalToCondensed[i]].uToken;
		/* If no %label has been specified, create the name from the
		   token name itself. */
		if (directive->token[1] == NULL) {
			char *text;
			if (option.lowercaseSymbols) {
				size_t length;
				text = safeStrdup(directive->token[0]->text, "generateDefaultGlobalDeclarations");
				length = strlen(text);
				for (j = 0; j < length; j++)
					text[j] = (char) tolower(text[j]);
			} else {
				text = directive->token[0]->text;
			}

			tfprintf(output, "\"%s\"", text);

			if (option.lowercaseSymbols)
				free(text);
		} else {
			if (option.gettext)
				tfprintf(output, "%s(", option.gettextMacro);
			tfputs(output, directive->token[1]->text);
			if (option.gettext)
				tfputc(output, ')');
		}
	}
	tfputs(output, "};\n");

	/* Make sure the macro doesn't interfere with our code (or the user's). */
	if (option.gettext)
		tfprintf(output, "#undef %s\n#ifdef %s\nchar *gettext(const char *);\n#else\n#define gettext(LLx) LLx\n#endif\n", option.gettextMacro, option.gettextGuard);

	tfprintf(output,
		"const char *LLgetSymbol(int LLtoken) {\n"
			"\tif (LLtoken < -1 || LLtoken > %d /* == LL_MAXTOKNO */)\n"
				"\t\treturn (char *) 0;\n", maxTokenNumber - 1);
	if (option.gettext)
		tfputs(output, "\treturn gettext(LLsymbolTable[LLtoken+1]);\n");
	else
		tfputs(output, "\treturn LLsymbolTable[LLtoken+1];\n");
	tfputs(output, "}\n");

	if (option.gettext)
		tfprintf(output, "#ifndef %s\n#undef gettext\n#endif\n", option.gettextGuard);
}

/** Write code to define all internal variables and utility functions.
	@param output The @a File to write to.
	@param headerName The name of the header file to include.

	These variables and functions end up in the .c file, or in the case of
	LLgen style outputs in the Lpars.c file.
*/
void generateDefaultGlobalDeclarations(File *output, const char *headerName) {
	const char *lexer, *lexerInternal = NULL; /* The = NULL is there to shut up the compiler */
	const char *storageType = option.LLgenStyleOutputs ? STORAGE_GLOBAL : STORAGE_STATIC;

	lexer = lexicalDirective ? lexicalDirective->token[0]->text : "yylex";
	if (option.generateLexerWrapper) {
		lexerInternal = lexer;
		lexer = "LLlexerWrapper";
	}

	generateDefaultDeclarations(output, storageType);
	tfputs(output, "#include <string.h>\n");

	generateConversionTables(output, storageType);

	/* Because LLmessage and the lexer(-wrapper) can be supplied externally,
	   these have to be made global. */
	tfputs(output, "int LLsymb;\n");
	if (!option.noLLreissue)
		tfprintf(output, "%sint LLreissue;\n", option.generateLexerWrapper ? STORAGE_STATIC : STORAGE_GLOBAL);

	/* Declare the lexer */
	tfprintf(output, "int %s(void);\n", lexerInternal != NULL ? lexerInternal : lexer);

	if (option.generateLexerWrapper) {
		tfputs(output, "static ");
		tfputs(output, lexerWrapperHeader);
		tfprintf(output, lexerWrapperString, "-2 /* LL_NEW_TOKEN */", lexerInternal, "", "-2 /* LL_NEW_TOKEN */");
	}
	/* As we generate our standard LLmessage as a static routine, we need to
	   declare it static here as well to prevent compiler warnings. */
	if (option.generateLLmessage)
		tfputs(output, "static ");
	/* Declare LLmessage (or actually <%prefix>message, because of the defines) */
	tfputs(output, "void LLmessage(int);\n");

	/* Define LLread. For correcting error recovery, it can be really simple.
	   As a good compiler will inline this anyway, we don't have to bother
	   with a #define. */
	tfprintf(output, "%svoid LLread(void) { LLcsymb = LLindex[(LLsymb = %s()) + 1]; }\n", storageType, lexer);

	/* Define the error handling routines LLskip and LLerror
	   Note: LLskip is spread over two tfprintf statements because otherwise
	   the format string would exceed the maximum length for a string constant.
	*/
	tfprintf(output,
		"%sint LLskip(void) {\n"
			"\tint LL_i;\n"
			"\tif (LLcsymb >= 0) {\n"
				"\t\tif (LLtcnt[LLcsymb] != 0) return 0;\n"
				"\t\tfor (LL_i = 0; LL_i < LL_NSETS; LL_i++)\n"
					"\t\t\tif (LLscnt[LL_i] != 0 && LLinset(LL_i))\n"
						"\t\t\t\treturn 0;\n"
			"\t}\n\n", storageType);
	tfprintf(output,
			"\tfor (;;) {\n"
				"\t\tLLmessage(%d /* LL_DELETE */);\n"
				"\t\twhile ((LLcsymb = LLindex[(LLsymb = %s()) + 1]) < 0) LLmessage(%d /* LL_DELETE */);\n"
				"\t\tif (LLtcnt[LLcsymb] != 0)\n"
					"\t\t\treturn 1;\n"
				"\t\tfor (LL_i = 0; LL_i < LL_NSETS; LL_i++)\n"
					"\t\t\tif (LLscnt[LL_i] != 0 && LLinset(LL_i))\n"
						"\t\t\t\treturn 1;\n"
		"\t}\n}\n", option.noEOFZero ? -2 : 0, lexer, option.noEOFZero ? -2 : 0);

	tfprintf(output,
		"%svoid LLerror(int LLtoken) {\n"
			"\tif (LLtoken == 256 /* EOFILE */) {\n"
				"\t\tLLmessage(-1 /* LL_MISSINGEOF */);\n"
				"\t\twhile (LLindex[(LLsymb = %s()) + 1] != 0) /*NOTHING*/ ;\n"
				"\t\treturn;\n"
			"\t}\n"
			"\tLLtcnt[LLindex[LLtoken + 1]]++;\n"
			"\tLLskip();\n"
			"\tLLtcnt[LLindex[LLtoken + 1]]--;\n"
			"\tif (LLsymb != LLtoken) {%s LLmessage(LLtoken); }\n"
		"}\n", storageType, lexer, option.noLLreissue ? "" : " LLreissue = LLsymb;");

	/* Generate the LLfirst routine, but only if a %first directive is used. */
	if (hasFirst) {
		tfprintf(output,
			"%sint LLfirst(int LLtoken, int LLset) {\n"
				"\tint LLctoken;\n"
				"\treturn (LLtoken >= -1 && LLtoken < %d && (LLctoken = LLindex[LLtoken+1]) >= 0 &&\n"
					"\t\t(LLsets[LLset*LL_SSETS + (LLctoken/8)] & (1<<(LLctoken & 7))));\n"
			"}\n", storageType, maxTokenNumber);
	}

	/* Generate the LLabort routine, but only if a the --abort option is used. */
	if (option.abort) {
		tfputs(output,
			"#include <setjmp.h>\n"
			"static jmp_buf LLjmp_buf;\n"
			"void LLabort(int LLretval) {\n"
				"\tlongjmp(LLjmp_buf, LLretval);\n"
			"}\n");
	}

	/* Generate the symbol table and LLgetSymbol routine, but only if a the
	   --generateSymbolTable option is used. */
	if (option.generateSymbolTable)
		generateSymbolTable(output);


	if (headerName != NULL) {
		/* Include our header file */
		tfprintf(output, "#include \"%s\"\n", headerName);
	}

	if (option.generateLLmessage) {
		tfputs(output, "#include <stdio.h>\nstatic ");
		tfputs(output, llmessageHeader);
		tfputs(output, llmessageString[0]);
		tfputs(output, llmessageString[1]);
	}
}

/*============== Code to generate the actual rules and parsers ================*/
/*===== General routines =====*/
/** Write code snippet for tracking the contains set.
	@param output The @a File to write to.
	@param tokens The set to push or pop.
	@param direction Whether to push or pop.
*/
void generateSetPushPop(File *output, const set tokens, PushDirection direction) {
	int size = setFill(tokens);
	/* For empty sets, there is nothing we need to do. */
	if (size == 0)
		return;
	/* Take note of the character after LL. */
	if (size == 1)
		tfprintf(output, "LLtcnt[%d]%s;\n", setFirstToken(tokens), direction == DIR_PUSH ? "++" : "--");
	else
		tfprintf(output, "LLscnt[%d]%s;\n", setFindIndex(tokens, false), direction == DIR_PUSH ? "++" : "--");
}

/** Write case labels for all tokens in a set.
	@param output The @a File to write to.
	@param tokens The set to write case labels for.

	The generated code makes extensive use of switch statements. This routine
	writes the case labels needed for one set.
*/
static void generateCaseLabels(File *output, const set tokens) {
	int i;
	for (i = 0; i < condensedNumber; i++) {
		if (setContains(tokens, i)) {
			tfprintf(output, "case %d:", i);
			tfprintTokenComment(output, i);
		}
	}
}

/*===== Code to generate parser =====*/
/** Write the code for a parser.
	@param output The @a File to write to.
	@param directive The %start directive to write code for.
*/
static void generateParser(File *output, Directive *directive) {
	NonTerminal *nonTerminal;

	nonTerminal = ((Declaration *) lookup(globalScope, directive->token[1]->text))->uNonTerminal;
	if (option.LLgenStyleOutputs)
		generateNonTerminalDeclaration(output, nonTerminal);

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

	if (option.abort)
		tfputs(output, "int LLsetjmpRetval;\n");
	if (option.reentrant) {
		const char *volatileMarker = "";

		/* According to the C rationale, after returning to a setjmp call context
		   through a longjmp call, the only way to make sure that all local
		   variables contain the values present at setjmp call time is to mark
		   the variables as volatile. Otherwise the last values written may be
		   in a register that is not restored after longjmp. */
		if (option.abort)
			volatileMarker = "volatile ";

		/* For reentrant parsers, the LLtcnt and LLscnt are actually
		   arrays on the stack, and the variables are pointers to this
		   stack array. */
		tfprintf(output, "\t%sint LLcounts[LL_NTERMINALS + LL_NSETS + 2];\n", volatileMarker);
		tfprintf(output, "\t%sint *LLbackupLLscnt = LLscnt, *LLbackupLLtcnt = LLtcnt;\n", volatileMarker);
		if (!option.noLLreissue)
			tfprintf(output, "\t%sint LLbackupLLreissue = LLreissue;\n", volatileMarker);
		if (option.abort)
			tfprintf(output, "\t%sjmp_buf LLbackupJmp_buf;\n\tmemcpy(&LLbackupJmp_buf, &LLjmp_buf, sizeof(jmp_buf));\n", volatileMarker);
		tfputs(output, "\tLLcounts[0] = LLsymb; LLcounts[1] = LLcsymb;\n");
		tfputs(output, "\tLLtcnt = LLcounts + 2; LLscnt = LLcounts + 2 + LL_NTERMINALS;\n");
	}
	/* Clear the contains set tracking variables. */
	tfputs(output, "\tmemset(LLscnt, 0, LL_NSETS * sizeof(int));\n");
	tfputs(output, "\tmemset(LLtcnt, 0, LL_NTERMINALS * sizeof(int));\n");
	/* EOFILE is always in the contains set: */
	tfputs(output, "\tLLtcnt[0]++;\n");

	tfputs(output, "\tLLsymb = 0;\n");

	if (!option.noLLreissue)
		tfputs(output, "\tLLreissue = -2 /* LL_NEW_TOKEN */;\n");

	if (option.abort)
		tfputs(output, "\tLLsetjmpRetval = setjmp(LLjmp_buf);\n\tif (LLsetjmpRetval == 0) {\n");
	tfputs(output, "\tLLread();\n");

	if (nonTerminal->term.flags & TERM_MULTIPLE_ALTERNATIVE)
		generateSetPushPop(output, nonTerminal->term.contains, DIR_PUSH);

	tfputc(output, '\t');
	if (nonTerminal->retvalIdent != NULL)
		tfputs(output, "*LLretval = ");

	tfprintf(output, "%s%d_%s();\n", prefix, nonTerminal->number, directive->token[1]->text);
	if (nonTerminal->term.flags & TERM_NO_FINAL_READ)
		tfputs(output, "\tLLread();\n");
	tfputs(output, "\tif (LLcsymb != 0) LLerror(256 /* EOFILE*/);\n");
#ifdef DEBUG
	tfputs(output, "\tLLtcnt[0]--;\n");
	tfputs(output, "{ int LL_i; for(LL_i = 0; LL_i < LL_NTERMINALS; LL_i++) if (LLtcnt[LL_i] != 0) { printf(\"Counts are not all zero!!!!\\n\"); exit(10); }}\n");
	tfputs(output, "{ int LL_i; for(LL_i = 0; LL_i < LL_NSETS; LL_i++) if (LLscnt[LL_i] != 0) { printf(\"Counts are not all zero!!!!\\n\"); exit(10); }}\n");
#endif
	if (option.abort)
		tfputs(output, "\t}\n");

	if (option.reentrant) {
		/* Restore the previous values */
		tfputs(output, "\tLLscnt = LLbackupLLscnt; LLtcnt = LLbackupLLtcnt;\n");
		tfputs(output, "\tLLsymb = LLcounts[0]; LLcsymb = LLcounts[1];\n");
		if (!option.noLLreissue)
			tfputs(output, "\tLLreissue = LLbackupLLreissue;\n");
		if (option.abort)
			tfputs(output, "\tmemcpy(&LLjmp_buf, &LLbackupJmp_buf, sizeof(jmp_buf));\n");
	}

	if (option.abort)
		tfputs(output, "\treturn LLsetjmpRetval;\n");
	tfputs(output, "}\n\n");
}

/** Write code for directives that can only appear at the end of the .c file.
	@param output The @a File to write to.
*/
void generateDirectiveCodeAtEnd(File *output) {
	int i;
	Declaration *declaration;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == DIRECTIVE && declaration->uDirective->subtype == START_DIRECTIVE)
			generateParser(output, declaration->uDirective);
	}
}
/*===== Code to generate rules =====*/
int labelCounter;
static void alternativeGenerateCode(File *output, Alternative *alternative, bool needsFinalRead);

/** Write the code for the @a Alternatives of a @a Term.
	@param output The @a File to write to.
	@param term The @a Term to write code for.
	@param canSkip Boolean to indicate whether the current token when taking
		the switch is not always valid, which may require jumping out of the
		switch and take it again after skipping the current token.
	@param popType One of POP_NEVER, POP_ALWAYS and POP_CONDITIONAL.
		Determines whether the contains set of the term is popped after the
		correct alternative has been determined. POP_ALWAYS is used for terms
		that don't repeat, POP_CONDITIONAL is used for terms with fixed
		repetition counts, while POP_NEVER is used for variable repetition
		count terms.
*/
void termGenerateCode(File *output, Term *term, const bool canSkip, PopType popType) {
	int i, j, label = 0; /* The = 0 is there to shut up the compiler */
	Alternative *alternative, *otherAlternative;
	set conflicts, temporary;
	CCode *lastExpression;
	int lastLabel;

	if (!(term->flags & TERM_MULTIPLE_ALTERNATIVE)) {
		/* This is mainly for the case where we have a single alternative that
		   has a repetition other than 1. */
		if (popType) {
			if (popType == POP_CONDITIONAL)
				tfputs(output, "if (!LL_i)\n");
			generateSetPushPop(output, term->contains, DIR_POP);
		}
		alternativeGenerateCode(output, (Alternative *) listIndex(term->rule, 0), !(term->flags & TERM_NO_FINAL_READ));
		return;
	}

	/* If the default case can skip the current token, it needs to be able to
	   jump out of the switch so that it can chose an alternative again. This
	   requires a label. */
	if (canSkip) {
		label = labelCounter++;
		tfprintf(output, "LL_%d:\n", label);
	}
	tfprintf(output, "switch (LLcsymb) {\n");

	conflicts = newSet(condensedNumber);
	temporary = newSet(condensedNumber);

	for (i = 0; i < listSize(term->rule); i++) {
		alternative = (Alternative *) listIndex(term->rule, i);
		/* For alternatives that have conflicting tokens, special handling is
		   required. */
		if (alternative->flags & ALTERNATIVE_IF) {
			setClear(conflicts);
			for (j = i + 1; j < listSize(term->rule); j++) {
				otherAlternative = (Alternative *) listIndex(term->rule, j);
				if (!setIntersectionEmpty(alternative->first, otherAlternative->first)) {
					setCopy(temporary, alternative->first);
					setIntersection(temporary, otherAlternative->first);
					setUnion(conflicts, temporary);
					if (otherAlternative->label < 0)
						otherAlternative->label = labelCounter++;
				}
			}

			/* The %if may have been for tokens that have already been handled
			   when processing a previous alternative (also with a %if). So
			   the conflicting set may be empty now. */
			if (!setEmpty(conflicts)) {
				/* Give the alternative a label, if it doesn't already have
				   one. */
				if (alternative->label < 0)
					alternative->label = labelCounter++;
				/* Remove conflicting tokens from first set of this alternative,
				   to prevent duplicate case labels. */
				setMinus(alternative->first, conflicts);

				while (!setEmpty(conflicts)) {
				/* Steps:
					- find a set of tokens that share conflicting alternatives (these can be put in one case)
					- generate case labels for that set
					- generate a sequence of "if (condition) goto label;" statements, ending with a "goto label;"
						for the final alternative.
					- remove conflicting tokens from first set of any affected alternatives
					- repeat until there are no more conflicting tokens
				*/
					setCopy(temporary, conflicts);
					for (j = i + 1; j < listSize(term->rule); j++) {
						otherAlternative = (Alternative *) listIndex(term->rule, j);
						if (!setIntersectionEmpty(temporary, otherAlternative->first)) {
							setIntersection(temporary, otherAlternative->first);
						}
					}
					/* Now we have a set with conflict tokens, which share
					   the same conflicting alternatives. */
					generateCaseLabels(output, temporary);
					lastExpression = alternative->expression;
					lastLabel = alternative->label;
					for (j = i + 1; j < listSize(term->rule); j++) {
						otherAlternative = (Alternative *) listIndex(term->rule, j);
						if (!setIntersectionEmpty(temporary, otherAlternative->first)) {
							setMinus(otherAlternative->first, temporary);
							tfputs(output, "if\n");
							outputCode(output, lastExpression, true);
							tfprintf(output, "goto LL_%d;\n", lastLabel);
							lastExpression = otherAlternative->expression;
							lastLabel = otherAlternative->label;
						}
					}
					tfprintf(output, "goto LL_%d;\n", lastLabel);
					setMinus(conflicts, temporary);
				}
			}
		}


		if (alternative->flags & ALTERNATIVE_DEFAULT) {
			tfprintf(output, "default:\n");
			if (canSkip) {
				tfprintf(output, "if (LLskip())\ngoto LL_%d;\n/*FALLTHROUGH*/\n", label);
				generateCaseLabels(output, alternative->first);
			}
		} else {
			/* This may not actually generate any case labels, i.e. if an
			   alternative only has tokens that conflict with other
			   alternatives. The alternative WILL have a regular label then. */
			generateCaseLabels(output, alternative->first);
		}
		if (alternative->label >= 0)
			tfprintf(output, "LL_%d:\n", alternative->label);
		if (popType) {
			/* For use in fixed maximum repetition loops. */
			if (popType == POP_CONDITIONAL)
				tfputs(output, "if (!LL_i)\n");
			generateSetPushPop(output, term->contains, DIR_POP);
		}
		alternativeGenerateCode(output, alternative, !(term->flags & TERM_NO_FINAL_READ));
		tfputs(output, "break;\n");
	}
	tfprintf(output, "}\n");
	deleteSet(conflicts);
	deleteSet(temporary);
}

/** Write code for the ..? operator.
	@param output The @a File to write to.
	@param term The @a Term to write code for.
*/
static void finoptGenerateCode(File *output, Term *term) {
	/* Info needed:
		conflict set from enclosing term

	  [ fixed:
		if (LL_i)
			goto first;
	  ]
		switch (LLcsymb) {
			default:
				LLskip...
			case follow-outer-only:	//only in follow set of outer term
				goto outLabel;
	  [ not fixed:
			case conflict-outer:	//only in conflict set of outer term
				if ((LL_checked = (.1. != 0) + 1) == 1)
					break;
				goto first; //generated only if other conflict cases follow
			case conflict-first-conflict-outer: //in both first set and conflict set of outer term
				if ((.2. || (LL_checked = (.1. != 0) + 1) == 1)
					break;
				goto first; //generated only if other conflict cases follow
	  ]
			case conflict-first-follow-outer:	//in first set and follow set of outer term
				if (!.2.)
					break;
	  [ not fixed:
			case decision-outer-only:	//only in decision set of outer term
	  ]
			case first:	//first set of this term
				...
		}
	*/
	Term *outerTerm = term->enclosing.alternative->enclosing;
	int label = labelCounter++, firstLabel = -1;
	bool needsGotoBeforeNewCase = false;
	set resultSet;

	/* The firstLabel is initially set to -1, because it may not be necessary
	   to generate the label at all. It all depends on any repetition conflicts
	   and whether it is a fixed repetition type. */

	/* Note that if this is in a fixed repetition, than the number of
	   repetitions will be greater than 1. In the event that the finopt
	   operator is specified within a single fixed repetition the finopt
	   operator will have been converted to a STAR operator. */
	if (outerTerm->repeats.subtype & FIXED) {
		ASSERT(outerTerm->repeats.number > 1);
		/* firstLabel is always -1 here. */
		firstLabel = labelCounter++;
		tfprintf(output, "if (LL_i) goto LL_%d;\n", firstLabel);
	}

	tfprintf(output, "LL_%d:\n"
		"switch (LLcsymb) {\n"
			"default:\n"
				"if (LLskip())\ngoto LL_%d;\n", label, label);

	resultSet = newSet(condensedNumber);

	/* =========================================================
	   Generate case labels for tokens only in the follow set of the outer term */
	setCopy(resultSet, outerTerm->follow);
	setMinus(resultSet, term->first);
	if (!setEmpty(resultSet)) {
		tfputs(output, "/*FALLTHROUGH*/\n");
		generateCaseLabels(output, resultSet);
	}

	generateSetPushPop(output, term->contains, DIR_POP);

	/* This jump out the switch needs to be here regardless of the tokens in the
	   outerTerm follow set, because of the default case. */
	tfputs(output, "break;\n");

	if (outerTerm->flags & TERM_REQUIRES_LLCHECKED) {
		/* =========================================================
		   Generate the case for tokens only in the conflict set of the
		   outer term */
		setCopy(resultSet, *(outerTerm->conflicts));
		setMinus(resultSet, term->first);

		if (!setEmpty(resultSet)) {
			generateCaseLabels(output, resultSet);
			tfputs(output, "if ((LL_checked = (\n");
			outputCode(output, outerTerm->expression, true);
			tfputs(output, "!= 0) + 1) == 1) {\n");
			generateSetPushPop(output, term->contains, DIR_POP);
			tfprintf(output, "break;\n}\n");
			needsGotoBeforeNewCase = true;
		}

		/* =========================================================
		   Generate the case for tokens in both the conflict set of the
		   outer term and the first set of this term */
		setCopy(resultSet, *(outerTerm->conflicts));
		setIntersection(resultSet, term->first);
		if (!setEmpty(resultSet)) {
			if (needsGotoBeforeNewCase) {
				if (firstLabel == -1)
					firstLabel = labelCounter++;
				tfprintf(output, "goto LL_%d;\n", firstLabel);
			}
			generateCaseLabels(output, resultSet);
			tfputs(output, "if ((\n");
			outputCode(output, term->expression, true);
			tfputs(output, "|| (LL_checked = (\n");
			outputCode(output, outerTerm->expression, true);
			tfputs(output, "!= 0) + 1) == 1)) {\n");
			generateSetPushPop(output, term->contains, DIR_POP);
			tfprintf(output, "break;\n}\n");
			needsGotoBeforeNewCase = true;
		}
	}

	/* =========================================================
	   Generate the case for tokens in both the follow set of the
	   outer term and the first set of this term */
	setCopy(resultSet, outerTerm->follow);
	setIntersection(resultSet, term->first);
	if (!setEmpty(resultSet)) {
		if (needsGotoBeforeNewCase) {
			if (firstLabel == -1)
				firstLabel = labelCounter++;
			tfprintf(output, "goto LL_%d;\n", firstLabel);
		}
		generateCaseLabels(output, resultSet);
		tfputs(output, "if (!\n");
		outputCode(output, term->expression, true);
		tfputs(output, ") {\n");
		generateSetPushPop(output, term->contains, DIR_POP);
		tfputs(output, "break;}\n");
		/* Eventhough this is the last case, we set the flag anyway, so that
		   we can properly generate the fallthrough. */
		needsGotoBeforeNewCase = true;
	}

	if (needsGotoBeforeNewCase)
		tfputs(output, "/*FALLTHROUGH*/\n");

	/* =========================================================
	   Generate the case for tokens in either the first set of the
	   outer term and the first set of this term */
	if (!(outerTerm->repeats.subtype & FIXED)) {
		if (outerTerm->repeats.subtype == PLUS && !(outerTerm->flags & TERM_PERSISTENT))
			setCopy(resultSet, outerTerm->first);
		else
			setCopy(resultSet, outerTerm->contains);

		setMinus(resultSet, term->first);
		if (outerTerm->flags & TERM_REQUIRES_LLCHECKED)
			setMinus(resultSet, *(outerTerm->conflicts));

		if (!setEmpty(resultSet))
			generateCaseLabels(output, resultSet);
	}

	/* Now for all tokens in the term's first set that have not been handled
	   in previous cases. */
	setCopy(resultSet, term->first);
	setMinus(resultSet, outerTerm->follow);
	if (outerTerm->flags & TERM_REQUIRES_LLCHECKED)
		setMinus(resultSet, *(outerTerm->conflicts));

	generateCaseLabels(output, resultSet);

	if (firstLabel != -1)
		tfprintf(output, "LL_%d:\n", firstLabel);

	generateSetPushPop(output, term->contains, DIR_POP);
	termGenerateCode(output, term, outerTerm->repeats.subtype & FIXED, POP_NEVER);

	tfputs(output, "}\n");
	deleteSet(resultSet);
}

/** Write the code for the repeat-structure of a @a Term.
	@param output The @a File to write to.
	@param term The @a Term to write code for.
*/
static void termGenerateRepeatsCode(File *output, Term *term) {
	/* Initialize the firstSetLabel only in the cases we actually need it */
	int firstSetLabel = (term->repeats.subtype & PLUS) ? labelCounter++ : -1;
	int braces = 0;

	if (term->repeats.number > 1 || term->flags & TERM_REQUIRES_LLCHECKED) {
		tfputc(output, '{');
		braces++;
	}

	if (term->repeats.number > 1) {
		/* Initialize it here, so that when we jump into the
		   for-loop (+ repetitions) it is initialized correctly. */
		tfprintf(output, "int LL_i = %d;\n", term->repeats.number - 1);
	}

	if (term->flags & TERM_REQUIRES_LLCHECKED) {
		/* Create flag for checking whether a first-follow conflict has
		   already been checked for. This flag has the following values:
		   - 0: no checking has been done.
		   - 1: checking has been done, and the result was false.
		   - 2: checking has been done, and the result was true.
		*/
		tfprintf(output, "int LL_checked = 0;\n");
	}

	/* For + repetitions we need to make sure we do any necessary
	   skips before jumping into the for-loop. Also, we need to do
	   the skips before changing from contains set to firstset
	   (if that is necessary). */
	if (term->repeats.subtype & PLUS) {
		tfputs(output, "switch (LLcsymb) {\n");
		generateCaseLabels(output, term->first);
		tfputs(output, "break;\n"
			"default: LLskip(); break;\n"
			"}\n");

		/* For persistent + repetitions the contains set equals
		   the first set. */
		if (!(term->flags & TERM_PERSISTENT)) {
			generateSetPushPop(output, term->contains, DIR_POP);
			generateSetPushPop(output, term->first, DIR_PUSH);
		}
		/* Jump into the for-loop, so that we at least do one
		   iteration. */
		tfprintf(output, "goto LL_%d;\n", firstSetLabel);
	}

	if (term->repeats.number < 0) {
		tfprintf(output, "for (;;) {\n");
		braces++;
	} else if (term->repeats.number > 1) {
		tfputs(output, "for(; LL_i >= 0; LL_i--) {\n");
		braces++;
	}

	if (term->repeats.subtype & (STAR|PLUS)) {
		int label = labelCounter++;
		set decisionSet, labelSet, conflicts;

		/* Note that because there is no switch surrounding the term
		   code, it may skip (at least the first time). */
		tfprintf(output, "LL_%d:\n"
			"switch (LLcsymb) {\n", label);

		if (term->repeats.subtype == PLUS && !(term->flags & TERM_PERSISTENT))
			decisionSet = term->first;
		else
			decisionSet = term->contains;
		labelSet = newSet(condensedNumber);
		setCopy(labelSet, decisionSet);

		/* =========================================================
		   Generate the code for tokens neither in the first nor
		   the follow set. */
		tfprintf(output, "default:\n"
			"if (LLskip()) goto LL_%d;\n", label);

		/* The fallthrough LLgen tries to support here is useless. The only time
		   it would do the fallthrough is the case where no tokens are skipped
		   and the current token is in the decision set. However, if that is
		   the case the switch would have jumped to the code rather than to the
		   default case. */
		if (term->repeats.subtype == STAR && term->repeats.number == 1)
			generateSetPushPop(output, decisionSet, DIR_POP);
		tfprintf(output, "break;\n");

		/* =========================================================
		   Generate code for tokens in the follow set. */

		/* If a repetition conflict was there, it also has a %while
		   associated with it. Therefore we need to remove the
		   conflicting tokens from the first and follow set, and
		   generate separate code for that. */
		if (term->flags & TERM_RCONFLICT) {
			conflicts = newSet(condensedNumber);
			/* Determine the set of conflicting tokens */
			setCopy(conflicts, term->follow);
			setIntersection(conflicts, labelSet);
			/* Remove conflicting tokens from first and follow set */
			setMinus(term->follow, conflicts);
			setMinus(labelSet, conflicts);
		}

		/* The follow set can be empty if only conflicting tokens
		   can follow the repetition. */
		if (!setEmpty(term->follow)) {
			generateCaseLabels(output, term->follow);
			if (term->repeats.subtype == STAR && term->repeats.number == 1)
				generateSetPushPop(output, decisionSet, DIR_POP);
			tfprintf(output, "break;\n");
		}

		if (term->flags & TERM_RCONFLICT) {
			/* Generate the code for the conflicting tokens */
			generateCaseLabels(output, conflicts);
			tfputs(output, "if (!(");
			if (term->flags & TERM_REQUIRES_LLCHECKED)
				tfputs(output, "LL_checked == 2 || (LL_checked == 0 &&");
			tfputc(output, '\n');
			outputCode(output, term->expression, true);
			if (term->flags & TERM_REQUIRES_LLCHECKED)
				tfputc(output, ')');
			tfputs(output, ")) {\n");
			if (term->repeats.subtype == STAR && term->repeats.number == 1)
				generateSetPushPop(output, decisionSet, DIR_POP);
			tfputs(output, "break;\n}\n");
			if (!setEmpty(labelSet))
				tfputs(output, "/*FALLTHROUGH*/\n");
		}

		/* =========================================================
		   Generate code for tokens in the first set. */

		/* Although the first set can be empty after removing any
		   conflicting tokens, we might as well just call
		   generateCaseLabels because the code following it will
		   need to be generated anyway. */
		generateCaseLabels(output, labelSet);
		deleteSet(labelSet);

		if (firstSetLabel >= 0)
			tfprintf(output, "LL_%d:\n", firstSetLabel);

		if (term->flags & TERM_REQUIRES_LLCHECKED)
			tfputs(output, "LL_checked = 0;\n");

		if (term->repeats.number > 1)
			tfputs(output, "if (!LL_i)\n");
		if (term->repeats.number >= 1)
			generateSetPushPop(output, decisionSet, DIR_POP);

		if (term->flags & TERM_CONTAINS_FINOPT) {
			term->conflicts = &conflicts;
		}

		termGenerateCode(output, term, false, POP_NEVER);
		if (!(term->repeats.subtype == STAR && term->repeats.number == 1)) {
			tfputs(output, "continue;\n}\n");
			generateSetPushPop(output, decisionSet, DIR_POP);
			tfputs(output, "break;\n");
		} else {
			tfputs(output, "}\n");
		}

		if (term->flags & TERM_RCONFLICT)
			deleteSet(conflicts);
	} else {
		termGenerateCode(output, term, true, term->repeats.number > 1 ? POP_CONDITIONAL : POP_ALWAYS);
	}
	/* Write the closing braces (if there are any). Write a \n
	   after the last one. */
	for (; braces > 1; braces--)
		tfputc(output, '}');
	if (braces)
		tfputs(output, "}\n");
}

/** Write code for an alternative.
	@param output The @a File to write to.
	@param alternative The @a Alternative to write code for.
	@param needsFinalRead Boolean to indicate whether the alternative should
		end with a LLread call.
*/
static void alternativeGenerateCode(File *output, Alternative *alternative, bool needsFinalRead) {
	int i;
	GrammarPart *grammarPart;
	bool needsRead = false;

	bool needsPushPop = false;

	/* Push contains sets for all parts, although refrain from pushing for the
	   first part. TERMs and non-terminals with multiple alternatives are
	   exceptions in this respect because they always need to be pushed. */
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		if (needsPushPop) {
			/* Don't push TERMs here, as they will be pushed outside this if */
			if (grammarPart->subtype & (PART_TERMINAL|PART_LITERAL))
				tfprintf(output, "LLtcnt[%d]++;\n", grammarPart->uTerminal.terminal);
			else if (grammarPart->subtype == PART_NONTERMINAL)
				generateSetPushPop(output, grammarPart->uNonTerminal.nonTerminal->term.contains, DIR_PUSH);
		} else if (grammarPart->subtype != PART_ACTION) {
			needsPushPop = true;
			/* This is here mainly for efficiency reasons. There are two
			   options:
			   - add an LLincr to the start of each rule, which may result in
			   an LLdecr in the calling rule followed by an LLincr in the rule
			   - always do the LLincr in the calling rule, thereby avoiding
			   possible LLdecr's */
			if (grammarPart->subtype == PART_NONTERMINAL && (grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_MULTIPLE_ALTERNATIVE))
				generateSetPushPop(output, grammarPart->uNonTerminal.nonTerminal->term.contains, DIR_PUSH);
		}
		/* TERMs alway need to be pushed due to the possibility of skipping
		   tokens. We don't want to skip anything in the TERMs contains set. */
		if (grammarPart->subtype == PART_TERM)
			generateSetPushPop(output, grammarPart->uTerm.contains, DIR_PUSH);
	}

	needsPushPop = false;
	for (i = 0; i < listSize(alternative->parts); i++) {
		grammarPart = (GrammarPart *) listIndex(alternative->parts, i);
		switch (grammarPart->subtype) {
			case PART_ACTION:
				outputCode(output, grammarPart->uAction, true);
				break;
			case PART_TERMINAL:
			case PART_LITERAL:
				if (needsRead)
					tfputs(output, "LLread();\n");
				if (needsPushPop)
					tfprintf(output, "LLtcnt[%d]--;\n", grammarPart->uTerminal.terminal);

				if (grammarPart->uTerminal.terminal == 0)
					tfputs(output, "\tif (LLcsymb != 0) LLerror(EOFILE);\n");
				else if (condensedToTerminal[grammarPart->uTerminal.terminal].flags & CONDENSED_ISTOKEN)
					tfprintf(output, "LL_SCANDONE(%d);", condensedToTerminal[grammarPart->uTerminal.terminal].uToken->uNumber);
				else
					tfprintf(output, "LL_SCANDONE(%d);", condensedToTerminal[grammarPart->uTerminal.terminal].uLiteral);

				tfprintTokenComment(output, grammarPart->uTerminal.terminal);
				needsRead = true;
				break;
			case PART_NONTERMINAL:
				if (needsRead)
					tfputs(output, "LLread();\n");
				/* As explained above (where sets are pushed), non-terminals
				   with multiple alternatives never need their contains sets
				   decremented here. */
				if (needsPushPop && !(grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_MULTIPLE_ALTERNATIVE))
					generateSetPushPop(output, grammarPart->uNonTerminal.nonTerminal->term.contains, DIR_POP);

				if (!(grammarPart->uNonTerminal.flags & CALL_LLDISCARD)) {
					if (grammarPart->uNonTerminal.retvalIdent)
						tfprintf(output, "%s = ", grammarPart->uNonTerminal.retvalIdent->text);
					else if (grammarPart->uNonTerminal.nonTerminal->retvalIdent)
						tfprintf(output, "%s = ", grammarPart->token->text);
				}

				tfprintf(output, "%s%d_%s", prefix, grammarPart->uNonTerminal.nonTerminal->number, grammarPart->token->text);
				/* Thread-safe parsers require LLthis as the first argument.
				   Therefore we need to handle call generation for thread-safe
				   parsers quite differently. */
				if (!option.threadSafe) {
					if (grammarPart->uNonTerminal.expression != NULL) {
						tfputc(output, '\n');
						outputCode(output, grammarPart->uNonTerminal.expression, true);
						tfputs(output, ";\n");
					} else {
						tfputs(output, "();\n");
					}
				} else {
					if (grammarPart->uNonTerminal.expression != NULL) {
						tfputs(output, "(LLthis");
						if (grammarPart->uNonTerminal.nonTerminal->argCount > 0)
							tfputc(output, ',');
						tfputc(output, '\n');
						outputCode(output, grammarPart->uNonTerminal.expression, false);
						tfputs(output, ");\n");
					} else {
						tfprintf(output, "(LLthis);\n");
					}
				}
				needsRead = grammarPart->uNonTerminal.nonTerminal->term.flags & TERM_NO_FINAL_READ;
				break;
			case PART_TERM:
				if (needsRead)
					tfputs(output, "LLread();\n");

				if (grammarPart->uTerm.repeats.subtype == FINOPT)
					finoptGenerateCode(output, &grammarPart->uTerm);
				else
					termGenerateRepeatsCode(output, &grammarPart->uTerm);

				needsRead = grammarPart->uTerm.flags & TERM_NO_FINAL_READ;
				break;
			default:
				PANIC();
		}
		/* If the part didn't require a pop and it isn't an action, then the
		   next part does need a pop. */
		if (!needsPushPop && grammarPart->subtype != PART_ACTION)
			needsPushPop = true;
	}
	if (needsRead && needsFinalRead)
		tfputs(output, "LLread();\n");
}

/** Generate code for a variable to hold return values.
	@param output The @a File to write to.
	@param retvalIdent The type of the variable.
	@param name The name of the variable.
*/
static void generateReturnVariable(File *output, Token *retvalIdent, const char *name) {
	tfprintDirective(output, retvalIdent);
	tfputs(output, retvalIdent->text);
	tfputc(output, '\n');
	tfprintDirective(output, NULL);
	tfprintf(output, "%s;\n", name);
}

/** Iterator routine to generate variables for return values.
	@param key The name of the variable.
	@param data The first @a grammarPart that names this variable.
	@param userData The @a File to write to.
*/
static void generateVariable(const char *key, void *data, void *userData) {
	GrammarPart *grammarPart = (GrammarPart *) data;

	generateReturnVariable((File *) userData, grammarPart->uNonTerminal.nonTerminal->retvalIdent, key);
}

/** Iterator routine to initialise generated variables for return values.
	@param key The name of the variable.
	@param data The first @a grammarPart that names this variable (unused).
	@param userData The @a File to write to.
*/
static void generateVariableInit(const char *key, void *data, void *userData) {
	/* Parameter data is unused in this function, so make the compiler shut up. */
	(void) data;

	tfprintf((File *) userData, "memset(&%s, 0, sizeof(%s));\n", key, key);
}

/** Write code for a non-terminal.
	@param output The @a File to write to.
	@param nonTerminal The @a NonTerminal to write code for.
*/
void generateNonTerminalCode(File *output, NonTerminal *nonTerminal) {
	bool memsetsGenerated = false;

	/* Don't generate code for unreachable non-terminals */
	if (!(nonTerminal->flags & NONTERMINAL_REACHABLE))
		return;

	labelCounter = 0;

	/* Generate the header for the function, with parameters if it has them */
	generateNonTerminalName(output, nonTerminal);
	if (!option.threadSafe)
		generateNonTerminalParameters(output, nonTerminal);
	else
		generateNonTerminalParametersTS(output, nonTerminal);

	tfprintf(output, "{\n");

	/* Declare variables for return values. */
	if (nonTerminal->retvalIdent != NULL)
		generateReturnVariable(output, nonTerminal->retvalIdent, "LLretval");

	iterateScope(nonTerminal->retvalScope, generateVariable, output);

	if (!option.noInitLLretval) {
		/* Clear LLretval and variables generated to hold return values, and open
		   a new scope so that the variables defined by the user will be defined
		   at the start of the scope. */
		if (nonTerminal->retvalIdent != NULL) {
			tfputs(output, "memset(&LLretval, 0, sizeof(LLretval));\n");
			memsetsGenerated = true;
		}

		if (iterateScope(nonTerminal->retvalScope, generateVariableInit, output))
			memsetsGenerated = true;

		if (memsetsGenerated)
			tfputs(output, "{\n");
	}

	/* Generate the declarations if available */
	if (nonTerminal->declarations != NULL)
		outputCode(output, nonTerminal->declarations, false);

	termGenerateCode(output, &nonTerminal->term, true, nonTerminal->term.flags & TERM_MULTIPLE_ALTERNATIVE ? POP_ALWAYS : POP_NEVER);

	if (memsetsGenerated)
		/* Close scope opened after clearing LLretval (if it was opened). */
		tfputs(output, "}\n");

	if (nonTerminal->retvalIdent != NULL)
		tfputs(output, "return LLretval;\n");

	tfputs(output, "}\n");
}
