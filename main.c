#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "constants.h"
#include "parsetools.h"

int main()
{
    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    // holds separated words based on whitespace
    char* lineWords[MAX_LINE_WORDS + 1];

    // Shell Prompt
    // printf("Welcome to SegFault's simple shell\npress CTRL + D to exit\n>");

    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    while( fgets(line, MAX_LINE_CHARS, stdin) ) {

        // Gets the number of words user entered
        int numWords = split_cmd_line(line, lineWords);

        // Retrieves commands from user input and executes them
        execute(lineWords, numWords);
    }

    return 0;
}
