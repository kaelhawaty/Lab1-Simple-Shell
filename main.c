#define TERMINATE 0
#define FAILED -1
/* Initial buffer size */
#define BUFF_SIZE 10
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

FILE *logFile;
char* getTime(){
    time_t t;
    time(&t);
    char* time = ctime(&t);
    int len = strlen(time);
    time[len - 1] = '\0';
    return time;
}
void interrupt_handler(int sig)
{
    if(sig != SIGCHLD){
        close();
        exit(TERMINATE);
    }
    int status;
    int pid;
    while (pid = waitpid(-1, &status, WNOHANG) > 0)
    {
        fprintf(logFile, "%s : Process with PID = %d returned with status = %d\n", getTime(), pid, status);
    }
}

char *read_line()
{
    char *line = NULL;
    int buffSize = 0;
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
    return args;
}

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
            // TODO: Check given a signal from a background process that it is still waits for the current process.
            waitpid(pid, &status, WUNTRACED);
            time_t t;
            time(&t);
            fprintf(logFile, "%s : Process with PID = %d returned with status = %d\n", getTime(), pid, status);
        }
    }
}

// Initialization, configs.. etc.
void init()
{
    signal(SIGCHLD, interrupt_handler);
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
void close(){
    fclose(logFile);
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
    close();
    return 0;
}