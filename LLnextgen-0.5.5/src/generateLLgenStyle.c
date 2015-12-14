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
#include <stdlib.h>

#include "generate.h"
#include "list.h"
#include "option.h"
#include "globals.h"
#include "os.h"
#include "lexer.h"

typedef struct {
	const char *sourceFileName;
	File *output;
	bool firstNonTerminal;
} FileListItem;

static List *outputs;
static File *hOutput, *cOutput; /* Lpars.[ch] */

/** Open all required output files
	
	Note: for LLgen output an output file is created for each input file. 
	Further, two files, <prefix>pars.h and <prefix>pars.c, are created.
*/
static void openOutputs(void) {
	const char *cExtension = "c", *hExtension = "h";
	FileListItem *currentItem;
	const char *inputName, *prefixText;
	char *outputName;
	size_t lenPrefix;
	int i;

	if (option.extensions) {
		char *extension = listIndex(option.extensions, 0);
		if (extension != NULL)
			cExtension = extension;
		if (listSize(option.extensions) > 1 && (extension = listIndex(option.extensions, 1)) != NULL)
			hExtension = extension;
	}
	
	outputs = newList();
	
	if (listSize(option.inputFileList) == 0) {
		ASSERT(option.outputBaseName != NULL);
		currentItem = (FileListItem *) safeMalloc(sizeof(FileListItem), "openOutputs");
		/* Note that for LLgen style outputs the use of include files is
		   prohibited, so fileName still points to the string "<stdin>". */
		currentItem->sourceFileName = fileName;
		currentItem->firstNonTerminal = true;
		outputName = createNameWithExtension(option.outputBaseName, cExtension);
		currentItem->output = checkAndOpenFile(outputName, true);
		listAppend(outputs, currentItem);
	} else {
		/* Open the different output files */
		for (i = 0; i < listSize(option.inputFileList); i++) {
			inputName = (const char *) listIndex(option.inputFileList, i);
			
			currentItem = (FileListItem *) safeMalloc(sizeof(FileListItem), "openOutputs");
			currentItem->sourceFileName = inputName;
			/* Set the flag that indicates that we haven't yet generated a
			   NonTerminal */
			currentItem->firstNonTerminal = true;
			
			/* For LLgen style outputs we never keep the directory, as multiple 
			   directories may be present for different files. */
			inputName = baseName(inputName);
			outputName = createNameWithExtension(inputName, cExtension);
			for (i = 0; i < listSize(outputs); i++) {
				FileListItem *itemToCheck = (FileListItem *) listIndex(outputs, i);
				if (strcmp(outputName, itemToCheck->output->name) == 0)
					fatal("Input files %s and %s map to the same output name.\n", itemToCheck->output->name, outputName);
			}
			currentItem->output = checkAndOpenFile(outputName, true);
			listAppend(outputs, currentItem);
		}
	}
	
	/* Open Lpars.h and Lpars.c, or appropriatly prefixed version of them */
	if (prefixDirective == NULL) {
		prefixText = "L";
		lenPrefix = 1;
	} else {
		prefixText = prefixDirective->token[0]->text;
		lenPrefix = strlen(prefixText);
	}

	outputName = (char *) safeMalloc(strlen(prefixText) + 6 + strlen(hExtension), "openOutputs");
	sprintf(outputName, "%spars.%s", prefixText, hExtension);
	hOutput = checkAndOpenFile(outputName, false);
	outputName = (char *) safeMalloc(strlen(prefixText) + 6 + strlen(cExtension), "openOutputs");
	sprintf(outputName, "%spars.%s", prefixText, cExtension);
	cOutput = checkAndOpenFile(outputName, true);
}

/** Find the output file for the specified input file
	@param filename The name of the input file
*/
static FileListItem *findOutput(const char *inputName) {
	static FileListItem *lastFound = NULL;
	int i;
	
	if (lastFound != NULL && lastFound->sourceFileName == inputName)
		return lastFound;
	
	for (i = 0; i < listSize(outputs); i++) {
		lastFound = (FileListItem *) listIndex(outputs, i);
		if (lastFound->sourceFileName == inputName)
			return lastFound;
	}
	PANIC();
}

/** Write code for code fragements and @a NonTerminal's to file

	Note: for LLgen style code generation, each @a NonTerminal is written to
	the output file corresponding to the input file in which it was declared.
*/
static void generateCode(void) {
	int i;
	Declaration *declaration;
	FileListItem *output;
	
	for (i = 0; i < listSize(declarations); i++) {
		declaration = (Declaration *) listIndex(declarations, i);
		switch (declaration->subtype) {
			case CODE:
				output = findOutput(declaration->uCode->fileName);
				outputCode(output->output, declaration->uCode, false);
				break;
			case DIRECTIVE:
				/* output = findOutput(declaration->uDirective->token[0]->fileName); */
				break;
			case NONTERMINAL:
				output = findOutput(declaration->uNonTerminal->token->fileName);
				/* Generate the declarations for all generated NonTerminal
				   functions right before the first one (for each file), so we
				   always declare them before use. */
				if (output->firstNonTerminal) {
					output->firstNonTerminal = false;
					generatePrototypes(output->output);
				}
				generateNonTerminalCode(output->output, declaration->uNonTerminal);
				break;
			default:
				PANIC();
		}
	}
}

/** Generate all output in LLnextgen style

	This function opens the output files, and writes all necessary output
	to them.
*/
void generateLLgenStyle(void) {
	int i;
	
	openOutputs();
	/* First generate all the default declarations and defines. */
	generateHeaderFile(hOutput);
	generateDefaultGlobalDeclarations(cOutput, NULL);
	/* We do this here, because for LLnextgen style output we don't want this
	   in generateDefaultGlobalDeclarations. */
	if (!option.noLLreissue)
		tfputs(cOutput, "#define LL_NEW_TOKEN (-2)\n");
	generateDirectiveCodeAtEnd(cOutput);
	
	/* These are necessary PER .c file */
	for (i = 0; i < listSize(outputs); i++) {
		FileListItem *lOutput = (FileListItem *) listIndex(outputs, i);
		generateDefaultLocalDeclarations(lOutput->output, hOutput->name);
		generateDirectiveDeclarations(lOutput->output);
	}
	/* Now where ready to generate the code. */
	generateCode();

	closeFile(hOutput, true);
	closeFile(cOutput, true);
	for (i = 0; i < listSize(outputs); i++) {
		FileListItem *lOutput = (FileListItem *) listIndex(outputs, i);
		closeFile(lOutput->output, true);
		free(lOutput);
	}
	deleteList(outputs);
}
