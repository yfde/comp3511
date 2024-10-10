/*
    COMP3511 Fall 2024
    PA1: Simplified Linux Shell (MyShell)

    Your name: Deng Yufan
    Your ITSC email: ydengbd@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

/*
    Header files for MyShell
    Necessary header files are included.
    Do not include extra header files
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls
#include <signal.h>   // For signal handling

// Define template strings so that they can be easily used in printf
//
// Usage: assume pid is the process ID
//
//  printf(TEMPLATE_MYSHELL_START, pid);
//
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"
#define TEMPLATE_MYSHELL_TERMINATE "Myshell (pid=%d) terminates by Ctrl-C\n"

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LENGTH 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the standard file descriptor IDs here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output
 
// This function will be invoked by main()
// This function is given
int get_cmd_line(char *command_line)
{
    int i, n;
    if (!fgets(command_line, MAX_CMDLINE_LENGTH, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(command_line);
    command_line[--n] = '\0';
    i = 0;
    while (i < n && command_line[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}

// read_tokens function is given
// This function helps you parse the command line
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char command_line[MAX_CMDLINE_LENGTH]; // The input command line
//
// Sample usage:
//
//  read_tokens(pipe_segments, command_line, &num_pipe_segments, "|");
//
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void process_cmd(char *command_line)
{
    // Uncomment this line to check the cmdline content
    // Please remember to remove this line before the submission
    // printf("Debug: The command line is [%s]\n", command_line);
    // int pfds[2]; 
    // pipe(pfds); 
    // pid_t pid = fork(); /* 0 (child), non-zero (parent) */ 
    // if ( pid == 0 ) { /* The child process*/ 
    //     close(1); /* close stdout */ 
    //     dup2(pfds[1],1); /* make stdout as pipe input */ 
    //     close(pfds[0]); /* don't need this */ 
    //     execlp("ls", "ls", NULL); 
    // } else { /* The parent process*/ 
    //     wait(0); /* wait for the child process */
    //     close(0); /* close stdin */ 
    //     dup2(pfds[0],0); /* make stdin as pipe output */ 
    //     close(pfds[1]); /* don't need this */
        
    //     execlp("wc", "wc", "-l", NULL); 
    // } 
    // start by parsing the command line
    char *pipe_segments[MAX_PIPE_SEGMENTS];
    int num_pipe_segments;
    read_tokens(pipe_segments, command_line, &num_pipe_segments, PIPE_CHAR);

    // execute the pipe segments in order
    for (int i = 0; i < num_pipe_segments; i++)
    {
        // print the index and the content of the current pipe segment
        // printf("Debug: Pipe segment %d: [%s]\n", i, pipe_segments[i]);

        // parse the arguments for the current pipe segment
        char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
        // null initialize the arguments array
        for (int j = 0; j < MAX_ARGUMENTS_PER_SEGMENT; j++)
        {
            arguments[j] = NULL;
        }
        int num_arguments;
        read_tokens(arguments, pipe_segments[i], &num_arguments, SPACE_CHARS);

        // if (num_arguments >= MAX_ARGUMENTS_PER_SEGMENT)
        // {
        //     arguments[MAX_ARGUMENTS_PER_SEGMENT - 1] = NULL;
        // }

        if (i != num_pipe_segments - 1)
        {
            // open pipe
            int pipefd[2];
            pipe(pipefd);

            // create a child process to execute the current pipe segment
            pid_t pid = fork();
            if (pid == 0)
            {
                // the child process executes the command
                // if (i != num_pipe_segments - 1)
                // {
                // close the read end of the pipe
                close(1);
                // redirect the stdout to the write end of the pipe
                dup2(pipefd[1], 1);
                // close the write end of the pipe
                close(pipefd[0]);
                // }
                // execute the command
                execvp(arguments[0], arguments);
                // if execvp returns, it must have failed
                // print an error message and exit
                perror("execvp");
                exit(1);
            }
            else
            {
                // the parent process waits for the child to finish
                wait(0);
                // if (i != num_pipe_segments - 1)
                // {
                // close the write end of the pipe
                close(0);
                // redirect the stdin to the read end of the pipe
                dup2(pipefd[0], 0);
                // close the read end of the pipe
                close(pipefd[1]);
                // }
            }



            // num_arguments = num_arguments >= MAX_ARGUMENTS_PER_SEGMENT ? MAX_ARGUMENTS_PER_SEGMENT-1 : num_arguments;
            // arguments[num_arguments] = NULL;

            // print the arguments for the current pipe segment
            // for (int j = 0; j < MAX_ARGUMENTS_PER_SEGMENT; j++)
            // {
            //     printf("Debug: Argument %d: [%s]\n", j, arguments[j]);
            // }

            // create a child process to execute the current pipe segment
            // pid_t pid = fork();
            // if (pid == 0)
            // {
            //     // the child process executes the command
            //     execvp(arguments[0], arguments);
            //     // if execvp returns, it must have failed
            //     // print an error message and exit
            //     perror("execvp");
            //     exit(1);
            // }
            // else
            // {
            //     // the parent process waits for the child to finish
            //     wait(0);
            // }
        }else{
            execvp(arguments[0], arguments);
        }
    }

}

void sigint_handler(int signo)
{
    // print the termination message
    printf(TEMPLATE_MYSHELL_TERMINATE, getpid());
    // exit the shell
    exit(1);
}

/* The main function implementation */
int main()
{
    // TODO: replace the shell prompt with your ITSC account name
    // For example, if you ITSC account is cspeter@connect.ust.hk
    // You should replace ITSC with cspeter
    char *prompt = "ydengbd";
    char command_line[MAX_CMDLINE_LENGTH];

    // register the signal handler for SIGINT
    signal(SIGINT, sigint_handler);

    // TODO:
    // The main function needs to be modified
    // For example, you need to handle the exit command inside the main function

    printf(TEMPLATE_MYSHELL_START, getpid());

    // The main event loop
    while (1)
    {

        printf("%s> ", prompt);
        if (get_cmd_line(command_line) == -1)
            continue; /* empty line handling */

        if (strcmp(command_line, "exit") == 0)
        {
            // exit the shell
            printf(TEMPLATE_MYSHELL_END, getpid());
            exit(0);
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            // the child process handles the command
            process_cmd(command_line);
        }
        else
        {
            // the parent process simply wait for the child and do nothing
            wait(0);
        }
    }

    return 0;
}
