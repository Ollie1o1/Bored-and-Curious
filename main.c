#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024  // Max input length
#define MAX_ARGS 64     // Max number of arguments

int main() {
    char input[MAX_INPUT];  // Store user input
    char *args[MAX_ARGS];   // Array for command and arguments
    char *token;            // For parsing input

    while (1) {
        // Print prompt
        printf("myshell> ");
        fflush(stdout);

        // Read input
        if (!fgets(input, MAX_INPUT, stdin)) {
            break; // Exit on EOF (Ctrl+D)
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Check for exit command
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Parse input into arguments
        int i = 0;
        token = strtok(input, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; // Null-terminate the argument list

        // Skip empty input
        if (i == 0) {
            continue;
        }

        // Fork to execute command
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            continue;
        } else if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                perror("Command execution failed");
            }
            exit(1); // Exit child if exec fails
        } else {
            // Parent process: wait for child
            int status;
            waitpid(pid, &status, 0);
        }
    }

    printf("Shell exiting...\n");
    return 0;
}