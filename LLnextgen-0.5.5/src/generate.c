/* Copyright (C) 2005,2006,2008,2011 G.P. Halkes
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

#include "list.h"
#include "generate.h"
#include "option.h"
#include "globals.h"
#include "io.h"
#include "os.h"

static File *cOutput, *hOutput;

/** Open all required output files.

	Note: for LLnextgen output there are only two output files, which names are
	determined from the first input file, or the user supplied option
	--base-name.
*/
static void openOutputs(void) {
	const char *inputName;
	char *hOutputName, *cOutputName;
	const char *cExtension = "c", *hExtension = "h";

	if (option.extensions) {
		char *extension = listIndex(option.extensions, 0);
		if (extension != NULL)
			cExtension = extension;
		if (listSize(option.extensions) > 1 && (extension = listIndex(option.extensions, 1)) != NULL)
			hExtension = extension;
	}

	/* Create the name of the .h file */
	if (listSize(option.inputFileList) == 0 || option.outputBaseName != NULL) {
		/* Add 1 for the dot, and 1 for the 0 byte */
		hOutputName = (char *) safeMalloc(strlen(option.outputBaseName) + 1 + strlen(hExtension) + 1, "openOutputs");
		cOutputName = (char *) safeMalloc(strlen(option.outputBaseName) + 1 + strlen(cExtension) + 1, "openOutputs");
		sprintf(hOutputName, "%s.%s", option.outputBaseName, hExtension);
		sprintf(cOutputName, "%s.%s", option.outputBaseName, cExtension);
	} else {
		inputName = (const char *) listIndex(option.inputFileList, 0);
		if (!option.keepDirInFilename)
			inputName = baseName(inputName);

		hOutputName = createNameWithExtension(inputName, hExtension);
		cOutputName = createNameWithExtension(inputName, cExtension);
	}

	hOutput = checkAndOpenFile(hOutputName, false);
	cOutput = checkAndOpenFile(cOutputName, true);
}

/** Write code for code fragements and @a NonTerminal's to file. */
static void generateCode(void) {
	int i;
	Declaration *declaration;
	bool firstNonTerminal = true;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		switch (declaration->subtype) {
			case CODE:
				outputCode(cOutput, declaration->uCode, false);
				break;
			case DIRECTIVE:
				break;
			case NONTERMINAL:
				/* Generate the declarations for all generated NonTerminal
				   functions right before the first one, so we always declare
				   them before use. */
				if (firstNonTerminal) {
					firstNonTerminal = false;
					generatePrototypes(cOutput);
				}
				generateNonTerminalCode(cOutput, declaration->uNonTerminal);
				break;
			default:
				PANIC();
		}
	}
}

/** Generate all output in LLnextgen style.

	This function opens the output files, and writes all necessary output
	to them.
*/
void generate(void) {
	openOutputs();
	if (top_code != NULL)
		outputCode(cOutput, top_code, false);
	/* First generate all the default declarations and defines. */
	generateHeaderFile(hOutput);
	generateDefaultGlobalDeclarations(cOutput, hOutput->name);
	/* Now where ready to generate the code. */
	generateDirectiveDeclarations(cOutput);
	generateCode();
	generateDirectiveCodeAtEnd(cOutput);

	closeFile(hOutput, true);
	closeFile(cOutput, true);
}


/** Write code for code fragements and @a NonTerminal's to file. */
static void generateCodeTS(void) {
	int i;
	Declaration *declaration;
	bool firstNonTerminal = true;

	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		switch (declaration->subtype) {
			case CODE:
				outputCode(cOutput, declaration->uCode, false);
				break;
			case DIRECTIVE:
				break;
			case NONTERMINAL:
				/* Generate the declarations for all generated NonTerminal
				   functions right before the first one, so we always declare
				   them before use. */
				if (firstNonTerminal) {
					firstNonTerminal = false;
					generatePrototypesTS(cOutput);
				}
				generateNonTerminalCode(cOutput, declaration->uNonTerminal);
				break;
			default:
				PANIC();
		}
	}
}

/** Generate all output in LLnextgen style for thread-safe parsers.

	This function opens the output files, and writes all necessary output
	to them.
*/
void generateTS(void) {
	openOutputs();
	if (top_code != NULL)
		outputCode(cOutput, top_code, false);

	if (dataTypeDirective != NULL) {
		userDataTypeTS = processString(dataTypeDirective->token[0]);
		if (dataTypeDirective->token[1] != NULL)
			userDataTypeHeaderTS = processString(dataTypeDirective->token[1]);
	}

	/* First generate all the default declarations and defines. */
	generateHeaderFileTS(hOutput);
	generateDefaultGlobalDeclarationsTS(cOutput, hOutput->name);
	/* Now where ready to generate the code. */
	generateDirectiveDeclarations(cOutput);
	generateCodeTS();
	generateDirectiveCodeAtEndTS(cOutput);

	closeFile(hOutput, true);
	closeFile(cOutput, true);
}
