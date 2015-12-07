SpecificationFile ->
    [SECTION_BEGIN OPTIONS OptionsSection SECTION_END OPTIONS SEMICOLON]? &&
    [SECTION_BEGIN DEFINES DefinesSection SECTION_END DEFINES SEMICOLON]? &&
    [SECTION_BEGIN REGEXPS RegexpsSection SECTION_END REGEXPS SEMICOLON];

OptionsSection ->
    [LEXER_OPTION IDENTIFIER SEMICOLON]? &&
    [LEXEME_OPTION IDENTIFIER SEMICOLON]? &&
    [[POSITIONING_OPTION POSITION_ON SEMICOLON
        WHERE_CLAUSE POSITIONING_LINE IDENTIFIER SEMICOLON
                     POSITIONING_COLUMN IDENTIFIER SEMICOLON] ||
    [POSITIONING_OPTION POSITIONING_OFF]] &&
    [DEFAULT_ACTION_OPTION IDENTIFIER SEMICOLON]?
    ;

DefinesSection ->
    [DEFINE IDENTIFIER EQUALS ???]*
    ;

RegexpsSection ->
    [REGEXP [??? | eof | anychar]
        [[TOKEN_DEF IDENTIFIER]|[NO_TOKEN_DEF]]
        [[ACTION_DEF IDENTIFIER]|[NO_ACTION_DEF]]?]+
