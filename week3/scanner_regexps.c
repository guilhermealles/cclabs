#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner_regexps.h"
#include "scanner_settings.h" // Criar biblioteca com definições!

#define MAX_REGEXP_SIZE 256
#define MAX_TOKEN_SIZE 128
#define BUFFER_SIZE 255

unsigned int regular_expressions_count = 0;
char** regular_expressions = NULL;
char** regular_expressions_tokens = NULL;
char** regular_expressions_actions = NULL;

void parseScannerRegularExpressions(FILE *file) {
    char* buffer = malloc(sizeof(char) * 255);
    int end_of_section = FALSE;
    int buffer_size = 0;
    int found_token = FALSE;
    int found_action = FALSE;

    regular_expressions = malloc(sizeof(char*) * 1);
    regular_expressions_tokens = malloc(sizeof(char*) * 1);
    regular_expressions_actions = malloc(sizeof(char*) * 1);


    fscanf(file, "%s", buffer);
    while (!end_of_section) {

        if(strcmp(buffer, "regexp") == 0){
            // Get the entire regular expression.
            fgets(buffer, MAX_REGEXP_SIZE+2, file);
            buffer_size = (int)strlen(buffer) + 1;

            regular_expressions = realloc(regular_expressions, sizeof(char*) * (regular_expressions_count+1));
            regular_expressions[regular_expressions_count] = malloc(sizeof(char*) * buffer_size);
            strcpy(regular_expressions[regular_expressions_count], buffer);

            fscanf(file, "%s", buffer);
            found_token = FALSE;
            found_action = FALSE;

            while(strcmp(buffer, "regexp") != 0 && strcmp(buffer, "end") != 0){
                if (strcmp(buffer, "token") == 0 || strcmp(buffer, "no") == 0 || strcmp(buffer, "action") == 0) {
                    if (strcmp(buffer, "token") == 0){
                        getNextSymbol(file, buffer, ';', TRUE);
                        buffer_size = (int)strlen(buffer) + 1;
                        regular_expressions_tokens = realloc(regular_expressions_tokens, sizeof(char*) * regular_expressions_count+1);
                        regular_expressions_tokens[regular_expressions_count] = malloc(sizeof(char*) * buffer_size);
                        strcpy(regular_expressions_tokens[regular_expressions_count], buffer);
                        found_token = TRUE;
                        // erro
                    }
                    else if (strcmp(buffer, "no") == 0){
                        getNextSymbol(file, buffer, ';', TRUE);
                        if (strcmp(buffer, "token") == 0){
                            // Ignore regular expression
                            char* token = "no token";
                            int token_size = (int)strlen(token) + 1;
                            regular_expressions_tokens = realloc(regular_expressions_tokens, sizeof(char*) * regular_expressions_count+1);
                            regular_expressions_tokens[regular_expressions_count] = malloc(sizeof(char*) * token_size);
                            strcpy(regular_expressions_tokens[regular_expressions_count], token);
                            found_token = TRUE;
                        }
                        else if(strcmp(buffer, "action") == 0){
                            char* action = "no action";
                            int action_size = (int)strlen(action) + 1;
                            regular_expressions_actions = realloc(regular_expressions_actions, sizeof(char*) * regular_expressions_count+1);
                            regular_expressions_actions[regular_expressions_count] = malloc(sizeof(char*) * action_size);
                            strcpy(regular_expressions_actions[regular_expressions_count], action);
                            found_action = TRUE;
                        }
                        else{
                            fprintf(stderr, "Error: unexpected keyword found \"%s\". \n", buffer);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (strcmp(buffer, "action") == 0){
                        getNextSymbol(file, buffer, ';', TRUE);
                        buffer_size = (int)strlen(buffer) + 1;
                        regular_expressions_actions = realloc(regular_expressions_actions, sizeof(char*) * regular_expressions_count+1);
                        regular_expressions_actions[regular_expressions_count] = malloc(sizeof(char*) * buffer_size);
                        strcpy(regular_expressions_actions[regular_expressions_count], buffer);
                        found_action = TRUE;
                    }


                }
                else{
                    fprintf(stderr, "Error: unexpected keyword found \"%s\".\n", buffer);
                    exit(EXIT_FAILURE);
                }
                fscanf(file, "%s", buffer);
            } // end while (buffer != regexp && buffer != end)

            if (!found_token){
                fprintf(stderr, "Error: token specification not found.\n");
                exit(EXIT_FAILURE);
            }
            if (! found_action){
                char* action = "default action";
                int action_size = (int)strlen(action) + 1;
                regular_expressions_actions = realloc(regular_expressions_actions, sizeof(char*) * regular_expressions_count+1);
                regular_expressions_actions[regular_expressions_count] = malloc(sizeof(char*) * action_size);
                strcpy(regular_expressions_actions[regular_expressions_count], action);
                found_action = TRUE;
            }
            regular_expressions_count++;


            if (strcmp(buffer, "end") == 0){
                end_of_section = TRUE;
            }
            else if(strcmp(buffer, "regexp") != 0){
                fprintf(stderr, "Parsing error.\n");
            }
        }

    }// end while(end of section)

    // Now expecting: "section regexps"
    fscanf(file, "%s", buffer);
    if (strcmp(buffer, "section") == 0){
        getNextSymbol(file, buffer, ';', TRUE);
        if (strcmp(buffer, "regexps") == 0){
            end_of_section = TRUE;
        }
        else{
            fprintf(stderr, "Error: expected end of section.\n");
        }
    }
    else{
        fprintf(stderr, "Error: expected end of section.\n");
    }
}

void printRegularExpressionsData(){
    int i;
    for (i = 0; i < regular_expressions_count; i++){
        printf("%d REGEXP: %s \n", i, regular_expressions[i]);
        printf("TOKEN: %s \n", regular_expressions_tokens[i]);
        printf("ACTION: %s \n", regular_expressions_actions[i]);
    }
}
