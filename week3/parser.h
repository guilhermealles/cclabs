/* THIS FILE HAS BEEN AUTOMATICALLY GENERATED BY LLnextgen. DO NOT EDIT */
#ifndef  __LLNEXTGEN_LL_H__
#define __LLNEXTGEN_LL_H__
#ifndef LL_NOTOKENS
#define EOFILE 256
#define BEGIN_SECTION_OPTIONS 257
#define BEGIN_SECTION_DEFINES 258
#define BEGIN_SECTION_REGEXPS 259
#define END_SECTION_OPTIONS 260
#define END_SECTION_DEFINES 261
#define END_SECTION_REGEXPS 262
#define SEMICOLON 263
#define COMMA 264
#define LEXER_OPTION 265
#define LEXEME_OPTION 266
#define POSITIONING_OPTION_ON 267
#define POSITIONING_OPTION_OFF 268
#define WHERE_CLAUSE 269
#define POSITIONING_LINE 270
#define POSITIONING_COLUMN 271
#define DEFAULT_ACTION_OPTION 272
#define DEFINE 273
#define EQUALS 274
#define TOKEN_DEF 275
#define NO_TOKEN_DEF 276
#define ACTION_DEF 277
#define NO_ACTION_DEF 278
#define REGEXP_EOF 279
#define REGEXP_ANYCHAR 280
#define REGEXP_DEF 281
#define EPSILON 282
#define OPEN_PARENTHESIS 283
#define CLOSE_PARENTHESIS 284
#define OPEN_CURLYBRACES 285
#define CLOSE_CURLYBRACES 286
#define OPEN_BRACES 287
#define CLOSE_BRACES 288
#define IDENTIFIER 289
#define LITERAL_INT 290
#define LITERAL_CHAR 291
#define RANGE_INT 292
#define RANGE_CHAR 293
#define OPERAND 294
#define BINARYOP 295
#define UNARYOP 296
#endif
#define LL_MAXTOKNO 296
#define LL_MISSINGEOF (-1)
#define LL_DELETE (0)
#define LL_VERSION 0x000505L
#define LL_NEW_TOKEN (-2)
void LLparser(void);
extern int LLsymb;
#endif
