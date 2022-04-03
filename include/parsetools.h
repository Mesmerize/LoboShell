#ifndef PARSETOOLS_H
#define PARSETOOLS_H

// Parse a command line into a list of words,
//    separated by whitespace.
// Returns the number of words
//

/* 
    Declaring struct to handle command arguments, handle flags, and be in charge of
    storing parsing info
*/

typedef struct command_arguments {
    char* indent;
    char* inFile;
    char* outFile;
    char** args;
    int inputFlag;
    int outputFlag;
    int counter;
    int append;
}   command_info;

// To handle Splitting of commands into an array
int split_cmd_line(char* line, char** list_to_populate); 
// To handle error checking
void syserror(const char* s);
// To handle checking for '<' operator
int isLessOp(const char* token);
// To handle checking for '|' operator
int isPipeOp(const char* token);
// To handle checking for '>' operator
int isGreaterOp(const char* token);
// To handle checking for '>>' append operator
int isAppendOp(const char* token);
// To handle valid command checking
int isValidCommand(const char* token);
// Handling execution logic
void execute(char* commandLine[], int numTokens);

#endif
