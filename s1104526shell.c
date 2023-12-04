#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_ARG_COUNT 16

void execute_command(char *args[], int background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        execvp(args[0], args);
        perror("Execution failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    }
}

void execute_pipeline(char *cmd1[], char *cmd2[]) {
    int pipe_fd[2];
    pid_t pid1, pid2;

    if (pipe(pipe_fd) < 0) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();
    if (pid1 < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        // Child process 1
        close(pipe_fd[0]); // Close unused read end
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        execvp(cmd1[0], cmd1);
        perror("Execution failed");
        exit(EXIT_FAILURE);
    } else {
        pid2 = fork();
        if (pid2 < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid2 == 0) {
            // Child process 2
            close(pipe_fd[1]); // Close unused write end
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);

            execvp(cmd2[0], cmd2);
            perror("Execution failed");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
    }
}

void redirect_input_output(char *input_file, char *output_file) {
    int input_fd, output_fd;

    if (input_file != NULL) {
        input_fd = open(input_file, O_RDONLY);
        if (input_fd < 0) {
            perror("Input file open failed");
            exit(EXIT_FAILURE);
        }
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    if (output_file != NULL) {
        output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (output_fd < 0) {
            perror("Output file open failed");
            exit(EXIT_FAILURE);
        }
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARG_COUNT];
    char *cmd1[MAX_ARG_COUNT];
    char *cmd2[MAX_ARG_COUNT];
    char *input_file = NULL;
    char *output_file = NULL;
    int background = 0;

    while (1) {
        printf("Shell> ");
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0'; // Remove trailing newline

        // Parse input
        char *token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Check for special command
        if (strcmp(args[0], "s1104526") == 0) {
            printf("Your UID is %d\n", getuid());
            continue;
        }

        // Check for background execution
        if (i > 0 && strcmp(args[i - 1], "&") == 0) {
            background = 1;
            args[i - 1] = NULL; // Remove the "&" from args
        }

        // Check for input/output redirection
        for (int j = 0; j < i; j++) {
            if (strcmp(args[j], "<") == 0) {
                input_file = args[j + 1];
                args[j] = NULL;
            } else if (strcmp(args[j], ">") == 0) {
                output_file = args[j + 1];
                args[j] = NULL;
            }
        }

        // Check for pipeline
        int pipe_index = -1;
        for (int j = 0; j < i; j++) {
            if (strcmp(args[j], "|") == 0) {
                pipe_index = j;
                break;
            }
        }

        if (pipe_index >= 0) {
            for (int j = 0; j < pipe_index; j++) {
                cmd1[j] = args[j];
            }
            cmd1[pipe_index] = NULL;

            int k = 0;
            for (int j = pipe_index + 1; j < i; j++) {
                cmd2[k++] = args[j];
            }
            cmd2[k] = NULL;

            execute_pipeline(cmd1, cmd2);
        } else {
            // Execute single process command
            redirect_input_output(input_file, output_file);
            execute_command(args, background);
        }

        // Reset variables for the next iteration
        input_file = NULL;
        output_file = NULL;
        background = 0;
    }

    return 0;
}
