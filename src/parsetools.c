#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include "constants.h"
#include "parsetools.h"



// Parse a command line into a list of words,
//    separated by whitespace.
// Returns the number of words
//
int split_cmd_line(char* line, char** list_to_populate) {
   char* saveptr;  // for strtok_r; see http://linux.die.net/man/3/strtok_r
   char* delimiters = " \t\n"; // whitespace
   int i = 0;

   list_to_populate[0] = strtok_r(line, delimiters, &saveptr);

   while(list_to_populate[i] != NULL && i < MAX_LINE_WORDS - 1)  {
       list_to_populate[++i] = strtok_r(NULL, delimiters, &saveptr);
   };

   return i;
}

void syserror(const char* s) {
    extern int errno;
    fprintf( stderr, "%s\n", s );
    fprintf( stderr, " (%s)\n", strerror(errno) );
    exit( 1 );
}

int isLessOp(const char* token) {
    // If token matches '<' operator then return true (1) 
    if(strcmp(token, "<") == 0) {
        return 1;
    } return 0; // Else false (0)
}

int isPipeOp(const char* token) {
    // If token matches '|' operator then return true (1) 
    if(strcmp(token, "|") == 0) {
        return 1;
    } return 0; // Else false (0)
}

int isGreaterOp(const char* token) {
    // If token matches '>' operator then return true (1) 
    if(strcmp(token, ">") == 0) {
        return 1;
    } return 0; // Else false (0)
}

int isAppendOp(const char* token) {
    // If token matches '>>' operator then return true (1) 
    if(strcmp(token, ">>") == 0) {
        return 1;
    } return 0; // Else false (0)
}

int isValidCommand(const char* token) {
    // If '<' or '>' or '|' is detected then return false (0) since it is not a command
    if(isLessOp(token) == 1 || isGreaterOp(token) == 1 || isPipeOp(token) == 1) {
        return 0;
    }   return 1;   // else return true (1)
}

void execute(char* commandLine[], int numTokens) {
    // Init Local Vars
    int fileValidator, ioValidator, commandCount, numPipes, firstCommand, args, append;
    append = fileValidator = ioValidator = commandCount = numPipes = 0;
    firstCommand = args = 1;

    // For loop traversing through commands
    for(int i  = 0; i < numTokens; i++) {
        // If pipe is detected then we increment number of pipes
        if(isPipeOp(commandLine[i]) == 1) {
            numPipes++;
        }
    }

    // Allocating memory to commandList (instance of struct)
    command_info* commandList = malloc((numPipes + 1) * sizeof(command_info));
    // Allocating memory to commandList index of args
    commandList[commandCount].args = malloc(numTokens * sizeof(char*));

    for(int i = 0; i < numTokens; i++) {

        if(firstCommand == 0) {
            if(isValidCommand(commandLine[i]) == 1) {
                if(fileValidator == 1) {
                    if (ioValidator == 0) {
                        strcpy((commandList[commandCount].outFile = malloc(strlen(commandLine[i]) + 1)), commandLine[i]);
                    }
                    else {
                        strcpy((commandList[commandCount].inFile = malloc(strlen(commandLine[i]) + 1)), commandLine[i]);
                    }
                }
                else {
                    strcpy((commandList[commandCount].args[args++] = malloc(strlen(commandLine[i]) + 1)), commandLine[i]);
                    commandList[commandCount].counter++;
                }
            }
            else if (isLessOp(commandLine[i]) == 1) {
                fileValidator = ioValidator = commandList[commandCount].inputFlag = 1;
            }
            else if (isGreaterOp(commandLine[i]) == 1) {
                fileValidator = commandList[commandCount].outputFlag = 1;
                ioValidator = 0;
            }
            else if (isAppendOp(commandLine[i]) == 1) {
                fileValidator = commandList[commandCount].append = 1;
                ioValidator = 0;
            }
            else if (isPipeOp(commandLine[i]) == 1) {
                commandList[++commandCount].args = malloc(numTokens * sizeof(char*));
                firstCommand = 1;
            }
        }

        else {
            if(isValidCommand(commandLine[i])){
                strcpy((commandList[commandCount].indent = malloc(strlen(commandLine[i]) + 1)), commandLine[i]);
                strcpy((commandList[commandCount].args[0] = malloc(strlen(commandLine[i]) + 1)), commandLine[i]);
                commandList[commandCount].counter = args = 1;
                commandList[commandCount].inputFlag = commandList[commandCount].outputFlag = fileValidator = firstCommand = 0;
            }
            else if(isPipeOp(commandLine[i])){
                commandList[++commandCount].args = malloc(sizeof(char*));
                firstCommand = 1;
            }
        }

    }

    int pfd[numPipes * 2];
    for (int i = 0; i < numPipes; i++){
        if (pipe(pfd + i * 2) < 0)
            syserror( "Error creating pipes" );
    }

    for (int i = 0; i < numPipes+1; i++) {
        commandList[i].args[commandList[i].counter] = NULL;
        int numWords = commandList[i].counter;
        for (int j = 0; j < numWords; j++) {
            if (commandList[i].args[j][0] == '\"' || commandList[i].args[j][0] == '\''){
                commandList[i].args[j]++;
                commandList[i].args[j][(strlen(commandList[i].args[j]-1)) - 2] = 0;
            }
        }

        int pid = fork();
        if(pid == -1){
            syserror( "Error creating fork" );
        }

        else if(pid == 0){
            if (commandList[i].outputFlag) {
                int fd;
                if ((fd = open(commandList[i].outFile, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0)
                    syserror("Error opening output file");
                if (dup2(fd, STDOUT_FILENO) < 0)
                    syserror("Error connecting STDOUT to file descriptor");
            }

            if (commandList[i].append) {
                int fd;
                if ((fd = open(commandList[i].outFile, O_WRONLY | O_APPEND, 0777)) < 0)
                    syserror("Error opening output file");
                if (dup2(fd, STDOUT_FILENO) < 0)
                    syserror("Error connecting STDOUT to file descriptor");
            }

            if (commandList[i].inputFlag) {
                int fd;
                if ((fd = open(commandList[i].inFile, O_RDONLY)) < 0)
                    syserror("Error opening input file");
                if (dup2(fd, STDIN_FILENO) < 0)
                    syserror("Error connecting STDIN to file descriptor");
            }
            if (i != 0)
                if (dup2(pfd[(i-1)*2], STDIN_FILENO) < 0)
                    syserror( "Error connecting STDIN to pipe read pipe" );
            if (i != numPipes)
                if (dup2(pfd[(i*2)+1], STDOUT_FILENO) < 0)
                    syserror( "Error connecting STDOUT to write pipe" );
            for (int i = 0; i < (numPipes) * 2; i ++)
                if (close(pfd[i]) < 0)
                    syserror( "Error closing pipes" );

            execvp(commandList[i].indent, commandList[i].args);

            syserror( "Error could not execvp" );

        }

    } // end of for loop

    for (int i = 0; i < (numPipes) * 2; i ++)
        if (close(pfd[i]) < 0)
            syserror( "Error closing remainder pipes" );

    while ( wait(NULL) != -1)
        ;

}