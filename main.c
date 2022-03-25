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

void syserror(const char *);

int main()
{
    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    // holds separated words based on whitespace
    char *line_words[MAX_LINE_WORDS + 1];

    printf("Welcome to SegFault's simple shell\npress CTRL + D to exit\n>");

    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    while (fgets(line, MAX_LINE_CHARS, stdin))
    {
        int num_words = split_cmd_line(line, line_words);

        for (int i = 0; i < num_words; i++)
        {
            printf("%s\n", line_words[i]);
        }
    }

    printf("Executing Command!\n");

    // testing pipe execution
    int pfd[2];
    pid_t pid, ret;

    if (pipe(pfd) == -1)
    {
        syserror("Could not create a pipe");
        return -1;
    }

    pid = fork();

    if (pid < 0)
    {
        syserror("Fork Failed!");
        return 1;
    }

    if (pid == 0)
    {
        printf("%d: I'm the child\n", getpid());
        fflush(stdout);
        dup(pfd[1]); // Writing into STDIN, left side of pipe
        close(pfd[1]);
        close(pfd[0]);
        execvp(line_words[0], line_words);
        perror("exec must have failed");
    }

    // Killing all kids
    close(pfd[1]);
    close(pfd[0]);

    pid = waitpid(-1, &ret, 0);
    // printf( "%d: parent done\n", getpid());
    // printf( "%d: child %d returned %d\n", getpid(), pid, WEXITSTATUS(ret));

    return 0;
}

void syserror(const char *s)
{
    extern int errno;
    fprintf(stderr, "%s\n", s);
    fprintf(stderr, " (%s)\n", strerror(errno));
    exit(1);
}