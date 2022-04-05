#ifndef PARSETOOLS_H
#define PARSETOOLS_H

// Parse a command line into a list of words,
//    separated by whitespace.
// Returns the number of words
//

// To handle Splitting of commands into an array
int split_cmd_line(char* line, char** list_to_populate); 
// To handle error checking
void syserror(const char* s, const char* cmd);
// Handling execution logic
void execute(char* commandLine[], int numTokens);

#endif
