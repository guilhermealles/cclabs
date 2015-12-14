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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "globals.h"
#include "lexer.h"
#include "option.h"
#include "generate.h"
#include "os.h"

void dependCpp(void) {
	int i;
	
	for (i = 0; i < listSize(declarations); i++) {
		Declaration *declaration = (Declaration *) listIndex(declarations, i);
		if (declaration->subtype == CODE)
			printf("#line %d \"%s\"\n%.*s\n", declaration->uCode->lineNumber, declaration->uCode->fileName,
					(int) strlen(declaration->uCode->text) - 2, declaration->uCode->text + 1);
	}
	exit(EXIT_SUCCESS);
}

static void printNamesLLgenStyle(FILE *output) {
	const char *inputName;
	char *outputName;
	int i;
	
	if (listSize(option.inputFileList) == 0) {
		ASSERT(option.outputBaseName != NULL);
		outputName = createNameWithExtension(option.outputBaseName, "c");
		fprintf(output, "%s ", outputName);
		free(outputName);
	} else {
		/* Print the different output files */
		for (i = 0; i < listSize(option.inputFileList); i++) {
			inputName = (const char *) listIndex(option.inputFileList, i);
			
			/* For LLgen style outputs we never keep the directory, as multiple 
			   directories may be present for different files. */
			inputName = baseName(inputName);
			outputName = createNameWithExtension(inputName, "c");

			fprintf(output, "%s ", outputName);
			free(outputName);
		}
	}
	
	/* Print Lpars.h and Lpars.c, or appropriatly prefixed version of them */
	if (prefixDirective == NULL) {
		fprintf(output, "Lpars.h Lpars.c");
	} else {
		fprintf(output, "%spars.h %spars.c", prefixDirective->token[0]->text, prefixDirective->token[0]->text);
	}
}

static void printNames(FILE *output) {
	const char *inputName;
	char *outputName;
	
	/* Create the name of the .h file */
	if (listSize(option.inputFileList) == 0 || option.outputBaseName != NULL) {
		/* Add 2 for the extension, and 1 for the 0 byte */
		fprintf(output, "%s.c %s.h", option.outputBaseName, option.outputBaseName);
	} else {
		inputName = (const char *) listIndex(option.inputFileList, 0);
		if (!option.keepDirInFilename)
			inputName = baseName(inputName);

		/* Create name with just a '.' at the end, so we can easily print it 
		   both with 'h' and 'c' appended */
		outputName = createNameWithExtension(inputName, "");
		
		fprintf(output, "%sc %sh", outputName, outputName);
		free(outputName);
	}
}

void depend(void) {
	FILE *output;
	int i;

	if (option.dependFile != NULL) {
		if ((output = fopen(option.dependFile, "w")) == NULL)
			fatal("Can't open output for dependency information: %s\n", strerror(errno));
	} else {
		output = stdout;
	}
	
	if (option.dependTargets == NULL) {
		if (option.LLgenStyleOutputs)
			printNamesLLgenStyle(output);
		else
			printNames(output);
	} else {
		fprintf(output, "%s", option.dependTargets);
	}

	if (option.dependExtraTargets != NULL)
		fprintf(output, " %s", option.dependExtraTargets);

	fputc(':', output);
	
	for (i = 0; i < listSize(option.inputFileList); i++) {
		const char *name = (const char *) listIndex(option.inputFileList, i);
		fprintf(output, " %s", name);
	}
	if (!option.LLgenStyleOutputs) {
		for (i = 0; i < listSize(includedFiles); i++) {
			const char *name = (const char *) listIndex(includedFiles, i);
			fprintf(output, " %s", name);
		}
	}
	fputc('\n', output);
	
	if (option.dependPhony) {
		fputc('\n', output);
		for (i = 0; i < listSize(option.inputFileList); i++) {
			const char *name = (const char *) listIndex(option.inputFileList, i);
			fprintf(output, "%s:\n\n", name);
		}
		if (!option.LLgenStyleOutputs) {
			for (i = 0; i < listSize(includedFiles); i++) {
				const char *name = (const char *) listIndex(includedFiles, i);
				fprintf(output, "%s:\n\n", name);
			}
		}
	}
	exit(EXIT_SUCCESS);
}
