#define TERMINATE 0
#define FAILED -1
/* Initial buffer size */
#define BUFF_SIZE 10
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

FILE *logFile;
char *getTime()
{
    time_t t;
    time(&t);
    char *time = ctime(&t);
    int len = strlen(time);
    time[len - 1] = '\0';
    return time;
}

void interrupt_handler(int sig)
{
    // If it is a termination signal, clean up and exit the program.
    if (sig != SIGCHLD)
    {
        fclose(logFile);
        exit(TERMINATE);
    }
    // Otherwise, it is a signal of a child getting terminated.
    int status;
    int pid;
    while (pid = waitpid(-1, &status, WNOHANG) > 0)
    {
        fprintf(logFile, "%s : Process with PID = %d returned with status = %d\n", getTime(), pid, status);
    }
}

// Reads the current command from standard input with no limit on the size and returns it. 
// Terminates the program if EOF is encountered or error while reading the line.
char *read_line()
{
    char *line = NULL;
    size_t buffSize = 0;
    // Read current command from stdin.
    if (getline(&line, &buffSize, stdin) == -1)
    {
        // Check if it is EOF.
        if (feof(stdin))
        {
            exit(TERMINATE);
        }
        else
        {
            perror("read_line: ");
            exit(FAILED);
        }
    }
    return line;
}
// Takes a command as string and splits into strings delimited by whitespaces which are returned as 
// char** -array of strings- dynamically resized to fit the number of arguments of any command.
// It additionally takes an integer pointer to return the number of arguments of the current command.
char **parse_command(char *line, int *arg_length)
{
    // 2D-array to store the splits of line around white spaces.
    char **args = malloc(BUFF_SIZE * sizeof(char *));
    int curSize = BUFF_SIZE;
    if (args == NULL)
    {
        perror("parse_command (Allocating): ");
        exit(FAILED);
    }
    char delimits[] = " \n";
    int it = 0;
    char *token = strtok(line, delimits);
    while (token != NULL)
    {
        args[it] = token;
        token = strtok(NULL, delimits);

        it++;
        if (it == curSize)
        {
            // Vector implementation: Multiply each time the size by two and reallocate more memory.
            curSize *= 2;
            args = realloc(args, curSize);
            if (args == NULL)
            {
                perror("parse_command (Reallocating): ");
                exit(FAILED);
            }
        }
    }
    *arg_length = it;
    args[it] = NULL;
    return args;
}

// Takes an array of strings resulting from parse_command function and the number of arguments. If it is “exit”,
// it terminates the program otherwise it forks the process and executes the command using execvp and either
// waits for the process or not depending on whether “&” argument is provided.
void execute_command(char **command, int arg_length)
{
    if (strcmp(command[0], "exit") == 0)
    {
        exit(TERMINATE);
    }

    bool run_in_background = false;
    if (strcmp(command[arg_length - 1], "&") == 0)
    {
        run_in_background = true;
    }
    int pid = fork();
    switch (pid)
    {
    // Child process.
    case 0:
        if (execvp(command[0], command) == -1)
        {
            perror("execute_command: ");
        };
        break;
    case -1:
        perror("fork: ");
        break;
    // Parent process.
    default:
        if (!run_in_background)
        {
            int status;
            // We use waitpid to wait for a specific process i.e the current one.
            // WUNTRACED: Reports on stopped child processes as well as terminated ones.
            waitpid(pid, &status, WUNTRACED);
            fprintf(logFile, "%s : Process with PID = %d returned with status = %d\n", getTime(), pid, status);
        }
    }
}

// Initialization, configs.. etc.
void init()
{
    signal(SIGCHLD, interrupt_handler);
    // Termination signal to properly flush logs before exiting.
    signal(SIGINT, interrupt_handler);
    signal(SIGQUIT, interrupt_handler);
    signal(SIGTERM, interrupt_handler);
    signal(SIGKILL, interrupt_handler);
    logFile = fopen("logs.txt", "w");
    if (logFile == NULL)
    {
        perror("init: ");
        exit(FAILED);
    }
}
int main()
{
    init();
    while (true)
    {
        printf("Shell>");
        char *line = read_line();
        int arg_length;
        char **command = parse_command(line, &arg_length);
        execute_command(command, arg_length);

        free(line);
        free(command);
    }
    return 0;
}