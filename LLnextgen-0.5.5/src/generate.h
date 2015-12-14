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

#ifndef GENERATE_H
#define GENERATE_H

#include "io.h"

#define STORAGE_STATIC "static "
#define STORAGE_EXTERN "extern "
#define STORAGE_GLOBAL ""

/* From generateCommon.c */
char *createNameWithExtension(const char *originalName, const char *extension);
void outputCode(File *output, CCode *code, bool keepMarkers);

void generateParserHeader(File *output, Directive *directive);
void generateParserDeclarations(File *output);

void generateHeaderFileTop(File *output);
void generateHeaderFile(File *output);

void generateDefaultPrefixTranslations(File *output);

void generateDefaultLocalDeclarations(File *output, const char *headerName);
void generateDirectiveDeclarations(File *output);
void generateNonTerminalDeclaration(File *output, NonTerminal *nonTerminal);
void generatePrototypes(File *output);

void generateConversionTables(File *output, const char *storageType);
void generateSymbolTable(File *output);

typedef enum {
	DIR_PUSH,
	DIR_POP
} PushDirection;

void generateSetPushPop(File *output, const set tokens, PushDirection direction);

void generateDefaultGlobalDeclarations(File *output, const char *headerName);

void generateDirectiveCodeAtEnd(File *output);

extern int labelCounter;
typedef enum {
	POP_NEVER = 0,
	POP_ALWAYS,
	POP_CONDITIONAL
} PopType;

void termGenerateCode(File *output, Term *term, const bool canSkip, PopType popType);

void generateNonTerminalCode(File *output, NonTerminal *nonTerminal);

/* From generateTS.c */
extern const char *userDataTypeTS, *userDataTypeHeaderTS;

void generateHeaderFileTS(File *output);
void generateNonTerminalParametersTS(File *output, NonTerminal *nonTerminal);
void generatePrototypesTS(File *output);
void generateDefaultGlobalDeclarationsTS(File *output, const char *headerName);
void generateDirectiveCodeAtEndTS(File *output);

/* From generate.c */
void generate(void);
void generateTS(void);

/* From generateLLgenStyle.c */
void generateLLgenStyle(void);


extern const char *lexerWrapperString,
	*llmessageString[2];
#endif
