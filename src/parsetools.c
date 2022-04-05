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

void syserror(const char* s, const char* cmd) {
    extern int errno;
    fprintf( stderr, "%s\n", s , cmd);
    fprintf( stderr, " (%s)\n", strerror(errno) );
    exit( 1 );
}


// commandline = lineWords; numTokens = numWords;
void execute(char* commandLine[], int numTokens) {

    int pfd1[2], pfd2[2];   // Init Pipes
    int firstCommand = 1;
    int i = 0;

    while(i < numTokens) {
        // First Worde in lineWords needs to be a function that is being called
        char *firstFunct = commandLine[i];
        char *functArgs[MAX_LINE_WORDS];    // Passing in function arguments, knowing that the first one is always a command
        functArgs[0] = commandLine[i];

        int argCount = 1; // Counter for lineWords[argCount], set to 1 because anything before 1 is a command
        pid_t pid;  // Init PID for use with fork

        int pipeSwitch = 0; // Used as a boolean to switch between creating first and second pipe
        int pipeGoingIn = 0;    // Another boolean used for children

        int hasToReadFirstPipe = 0; // If set to 1 we need to read still, 0 if we do not
        int hasReadFirstPipe = 0;   // If set to 1 we are about to read first pipe, 0 if child processes have read
        int hasToWriteFirstPipe = 0;    //If set to 1 we need to write it
        int hasToReadSecondPipe = 0;    // If set to 1 we need to read from second
        int hasReadSecondPipe = 0;  // If set to 1 we are about to read second pipe, 0 if child processes have read
        int hasToWriteSecondPipe = 0;   // If set to 1 we need to write to second pipe

        int inRedirect = 0; // Input Redirect Bool
        int outRedirect = 0;    // Output Redirect Bool
        int quoteParsing = 0;   // Used to aid in Parsing Quote
        int errCheck;
        int noWait = 0; // If a command is invalid we shall use this to determine not to wait

        // Triggers if we have a command
        if(numTokens > 1) {
            errCheck = 0;
            char *quoteString = NULL;   // Used for Quotes
            int fd; // Used when handling File Descriptors for input redirect
            int outFD;  // Used when handling File Descriptors for output redirect

            for(int j = i + 1; j < numTokens + 1; j++) {    // Looping through the entire Command

                if(argCount == 0) { // Reset Arg Counter if Triggered For Next Line in CLI
                    firstFunct = commandLine[j];
                }

                if (commandLine[j] == NULL || *(commandLine[j]) == PIPE || *(commandLine[j]) == INPUT_REDIRECT ||
                        *(commandLine[j]) == OUTPUT_REDIRECT || commandLine[j] == OUTPUT_APPEND) {

                            if(commandLine[j] == NULL) {
                                // If subsequent item is NULL
                                if(hasToReadFirstPipe) {
                                    pipeGoingIn = 1;
                                    pipeSwitch = 1;
                                    hasReadFirstPipe = 1;   // Since we have read first pipe, bool will be set to true
                                }
                                else if(hasToReadSecondPipe) {
                                    pipeGoingIn, pipeSwitch, hasReadSecondPipe = 1;
                                }
                            }

                            else if(*(commandLine[j]) == PIPE) {
                                // Finding a pipe and two pipes that were used previously will be reused
                                if(pipeSwitch) {
                                    pipeGoingIn = 1;

                                    if(pipe(pfd2) == -1) {
                                        syserror("Could not create a pipe", "");
                                    } else {
                                        hasToReadSecondPipe = 1;
                                        hasToWriteSecondPipe = 1;

                                        if(hasToReadFirstPipe) {
                                            hasReadFirstPipe = 1;   // Since we have read first pipe, bool will be set to true
                                        }
                                    }
                                }

                                else {
                                    // If Pipe was not set, we need to create that first pipe
                                    pipeSwitch = 1;
                                    pipeGoingIn = 1;

                                    if(pipe(pfd1) == -1) {
                                        syserror("Could not create a pipe", "");
                                    } 

                                    hasToReadFirstPipe = 1;
                                    hasToWriteFirstPipe = 1;

                                    if(hasToReadSecondPipe) {
                                        hasReadSecondPipe = 1;
                                    }
                                }
                            }

                            else if(*(commandLine[j]) == INPUT_REDIRECT) {
                                // Stroing input filename
                                char *inputFilename = commandLine[++j];
                                // Checking for pipe or output redirection
                                char *outputRedirect_or_Pipe = commandLine[++j];

                                int output = j;

                                if(outputRedirect_or_Pipe == NULL) {
                                    // Meh Blank, prevents errors
                                }

                                else if(*outputRedirect_or_Pipe == PIPE) {
                                    // If pipe found we use two pipes
                                    if(pipeSwitch) {
                                        // If a pipe already exists we need to make a second pipe
                                        pipeGoingIn = 1;

                                        if(pipe(pfd2) == -1) {
                                            syserror("Could not create a pipe", "");
                                        } else {
                                            hasToReadSecondPipe = 1;
                                            hasToWriteSecondPipe = 1;

                                            if(hasToReadFirstPipe) {
                                                hasReadFirstPipe = 1;   // Since we have read first pipe, bool will be set to true
                                            }
                                        }
                                    }

                                    else {
                                        // If pipe was not created, we need to create first pipe
                                        pipeSwitch = 1;
                                        pipeGoingIn = 1;

                                        if(pipe(pfd1) == -1) {
                                            syserror("Could not create a pipe", "");
                                        }

                                        hasToReadFirstPipe = 1;
                                        hasToWriteFirstPipe = 1;

                                        if(hasToReadSecondPipe) {
                                            hasReadSecondPipe = 1;
                                        }
                                    }
                                }

                                 else if (*outputRedirect_or_Pipe == OUTPUT_REDIRECT) {
                                     char *outputFilename = commandLine[++j];

                                     if(strcmp(commandLine[output], OUTPUT_APPEND) == 0) {
                                         outFD = open(outputFilename, O_WRONLY | O_CREAT | O_APPEND, 0664); // 0664 = user: r-w, group: r-w, other: r
                                     }

                                     else if(*(commandLine[output]) == OUTPUT_REDIRECT) {
                                         outFD = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC, 0664);  // 0664 = user: r-w, group: r-w, other: r
                                     }

                                     errCheck = errno;  // Err handling
                                     if(errCheck && errCheck != 10) {   // Error with no child processes
                                        fprintf(stderr, "-lobo-shell-198: %s: %s\n", outputFilename, strerror(errCheck)); // error checking
                                        noWait = 1;
                                     }

                                     outRedirect = 1;
                                 }

                                 fd = open(inputFilename, O_RDONLY);    // File Descriptor for [ command[args] + < inputFile]
                                 errCheck = errno;  // Err handling
                                 if(errCheck && errCheck != 10) {   // Error with no child processes
                                    fprintf(stderr, "-lobo-shell-198: %s: %s\n", inputFilename, strerror(errCheck)); // error checking
                                    noWait = 1;
                                 }

                                 inRedirect = 1;
                            }

                            else if (*(commandLine[j]) == OUTPUT_REDIRECT || commandLine[j] == OUTPUT_APPEND) {
                                int output = j;
                                char *outputFilename = commandLine[++j];
                                // Checking for output redirection or pipe
                                char *inputRedirect_or_Pipe = commandLine[++j];
                                if(inputRedirect_or_Pipe == NULL) {
                                    // Meh Blank, prevents errors
                                }

                                else if(*inputRedirect_or_Pipe == PIPE) {
                                    // Found a Pipe , Two Pipes we be reutilized
                                    if(pipeSwitch) {
                                        pipeGoingIn = 1;

                                        if(pipe(pfd2) == -1) {
                                            syserror("Could not create a pipe", "");
                                        } else {
                                            hasToReadSecondPipe = 1;
                                            hasToWriteSecondPipe = 1;

                                            if(hasToReadFirstPipe) {
                                                hasReadFirstPipe = 1;
                                            }
                                        }
                                    }

                                    else {
                                        // if pipe was not made / set, create first pipe
                                        pipeSwitch = 1;
                                        pipeGoingIn = 1;

                                        if(pipe(pfd1) == -1) {
                                            syserror("Could not create a pipe", "");
                                        }

                                        hasToReadFirstPipe = 1;
                                        hasToWriteFirstPipe = 1;

                                        if(hasToReadSecondPipe) {
                                            hasReadSecondPipe = 1;
                                        }
                                    }
                                }

                                else if(*inputRedirect_or_Pipe == INPUT_REDIRECT) {
                                    char *inputFilename = commandLine[++j];
                                    fd = open(inputFilename, O_RDONLY);

                                    if(errCheck && errCheck != 10) {   // Error with no child processes
                                        fprintf(stderr, "-lobo-shell-198: %s: %s\n", inputFilename, strerror(errCheck)); // error checking
                                        noWait = 1;
                                    }
                                    inRedirect = 1;
                                }

                                char *outApp = commandLine[output];
                                if(strcmp(outApp, OUTPUT_APPEND) == 0) {
                                    outFD = open(outputFilename, O_WRONLY | O_CREAT | O_APPEND, 0664);
                                }
                                else if(*(commandLine[output]) == OUTPUT_REDIRECT) {
                                    outFD = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
                                }

                                errCheck = errno;   // Err handling
                                if(errCheck && errCheck != 10) {   // Error with no child processes
                                   fprintf(stderr, "-lobo-shell-198: %s: %s\n", outputFilename, strerror(errCheck)); // error checking
                                   noWait = 1;
                                }
                                outRedirect = 1;
                            }

                            // Set last argument to NULL
                            functArgs[argCount] = NULL;
                            argCount = 0;   // Reset argument count for next command in the line if it exists

                            switch(pid = fork()) {
                                case -1:
                                    syserror("First fork failed", "");
                                    break;
                                case 0:
                                    if(pipeGoingIn) {   // We are using pipes
                                            if(!firstCommand) {
                                                /* 
                                                    If it is not the first command, We want it to be read from the
                                                    end of the pipe. So we either need to read from first pipe and not from second etc
                                                */
                                                if(hasToReadFirstPipe && !hasReadSecondPipe) {
                                                    // Reading from first pipe
                                                    dup2(pfd1[0], 0);   // reading into STDOUT, right side of pipe
                                                    close(pfd1[0]); // Close uneccesary fd

                                                    if(hasToWriteSecondPipe) {
                                                        dup2(pfd2[1], 1);  // Writing into STDIN, left side of pipe
                                                        close(pfd2[1]);
                                                        close(pfd2[0]);
                                                    }

                                                } else if(hasToReadSecondPipe && !hasReadFirstPipe) {
                                                    dup2(pfd2[0], 0);   // reading into STDOUT, right side of pipe
                                                    close(pfd2[0]);

                                                    // If need to write to first pipe
                                                    if(hasToWriteFirstPipe) {
                                                        dup2(pfd1[1], 1);
                                                        close(pfd1[1]);
                                                        close(pfd1[0]);
                                                    }
                                                } else if(outRedirect) {
                                                    dup2(outFD, 1);
                                                    close(outFD);
                                                }
                                            } else {
                                                if(hasToWriteFirstPipe) {
                                                    dup2(pfd1[1], 1);
                                                    close(pfd1[1]);
                                                    close(pfd1[0]);
                                                }
                                            }
                                    }

                                    if(inRedirect && !noWait) {
                                        dup2(fd, 0);
                                        if(outRedirect) {
                                            dup2(outFD, 1);
                                            close(outFD);
                                        } close(fd);
                                    }

                                    else if(outRedirect && !noWait) {
                                        dup2(outFD, 1);
                                        if(inRedirect) {
                                            dup2(fd, 0);
                                            close(fd);
                                        } close(outFD);
                                    }

                                    if(!noWait) {
                                        execvp(firstFunct, functArgs);
                                        syserror("Could not exec: ", firstFunct);
                                    } else {
                                        exit(0);
                                    }
                                    break;
                            default:
                                noWait = 0;
                                pipeGoingIn = 0;
                                if(pipeSwitch) {
                                    if(hasReadFirstPipe) {
                                        close(pfd1[0]); // If we have read from first pipe close fd
                                        hasReadFirstPipe = 0;
                                        hasToReadFirstPipe = 0;
                                    } else if(hasToReadFirstPipe) {
                                        close(pfd1[1]); // Close the write end of first pipe
                                        pipeGoingIn = 1;
                                    } if(hasToWriteSecondPipe) {
                                        // Indicates second pipe was already written to
                                        hasToWriteSecondPipe = 0;
                                        close(pfd2[1]);
                                    } if(hasToWriteFirstPipe) {
                                        // Indicates first pipe was already written to
                                        hasToWriteFirstPipe = 0;
                                    } if(hasReadSecondPipe) {
                                        close(pfd2[0]);
                                        hasReadSecondPipe = 0;
                                        hasToReadSecondPipe = 0;
                                    } else if(hasToReadSecondPipe) {
                                        close(pfd2[1]); // If we have read from second pipe close fd
                                        pipeSwitch = 0;
                                        pipeGoingIn = 1;
                                    }
                                }

                                firstCommand = 0;   // Not used as first command anymore

                                if(inRedirect) {    // If input redirect, close fd
                                    close(fd);
                                } if(outRedirect) { // If output redirect, close fd
                                    close(outFD);
                                }

                                memset(functArgs, 0, sizeof(functArgs));    // Reset arg array after waiting for children
                                errCheck = 0;   // Reset to 0 since we already closed a fd and errno would be -1
                                break;
                            } 
                        }   else {
                                // We build arg array
                                if (*commandLine[j] == '"' || *commandLine[j] == '\'') {
                                    quoteString = malloc(10000);
                                    quoteParsing = 1;
                                    int finishQuoteParsing = 0;

                                    int x = 1;
                                    int y = 0;

                                    char *charsInQuotes = commandLine[j];   // Gets first word ex: \"
                                    char c  = charsInQuotes[x];

                                    while(c != '"' && c != '\'') {
                                        while(!finishQuoteParsing) {
                                            while(c != '\0' && !finishQuoteParsing) {
                                                quoteString[y] = c;
                                                c = charsInQuotes[++x];
                                                ++y;
                                                if (c == '"' || c == '\'') {
                                                    finishQuoteParsing = 1;
                                                }
                                            } if(!finishQuoteParsing) {
                                                quoteString[y] = ' ';
                                                ++y;
                                                charsInQuotes = commandLine[++j];
                                                x = -1;
                                                c = charsInQuotes[++x];
                                            }
                                        }
                                    }

                                    functArgs[argCount] = quoteString;
                                    argCount++;
                                } else {
                                    functArgs[argCount] = commandLine[j];
                                    argCount++;
                                }
                            }
                            i = j;
                    }
                    while(waitpid(-1, NULL, 0) != -1); // See child pid that was return, if any

                    if (quoteParsing) {
                        memset(quoteString, 0, sizeof(quoteString));
                        free(quoteString);
                        quoteParsing = 0;
                    }
            }

            // single command, no arguments ex: whoami, ls, pwd
            else {
                switch (pid = fork()) {
                    case -1:
                        syserror("First fork failed", "");
                        break;
                    case 0:
                        execlp(firstFunct, firstFunct, NULL);
                        syserror("Could not exec: ", firstFunct);
                        break;
                    default:
		                //fprintf(stderr, "The child's pid is: %d\n", pid);
                        waitpid(pid,NULL, 0);
                        break;
                }
            }

            i++;
        }
}