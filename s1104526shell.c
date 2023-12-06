#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>  // Added this line

#define MAX_LINE 80
#define MAX_NUM_ARGS 10

size_t string_parser(char* input, char* word_array[]) {
    size_t n = 0;
    while (*input) {
        while (isspace((unsigned char)*input))
            ++input;
        if (*input) {
            word_array[n++] = (char*)input;
            while (*input && !isspace((unsigned char)*input))
                ++input;
            *(input) = '\0';
            ++input;
        }
    }
    word_array[n] = NULL;
    return n;
}
// Rest of the code...

void execute_command(char** args, int background) {
    pid_t pid;
    int status;
    int out = -1;
    int in = -1;

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            args[i] = NULL;
            out = open(args[i + 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            break;
        }
        if (strcmp(args[i], "<") == 0) {
            args[i] = NULL;
            in = open(args[i + 1], O_RDONLY);
            break;
        }
    }

    pid = fork();
    // Rest of the code...
    if (pid == 0) {
        if (out != -1) {
            dup2(out, STDOUT_FILENO);
            close(out);
        }
        if (execvp(args[0], args) == -1) {
            perror("1104526shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("1104526shell");
    } else {
        if (!background) {
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
}

void execute_pipe(char** args, char** args_pipe) {
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nCould not fork");
        return;
    }

    if (p1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(args[0], args) < 0) {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    } else {
        p2 = fork();

        if (p2 < 0) {
            printf("\nCould not fork");
            return;
        }

        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(args_pipe[0], args_pipe) < 0) {
                printf("\nCould not execute command 2..");
                exit(0);
            }
        } else {
            wait(NULL);
            wait(NULL);
        }
    }
}

void execute_s1104526() {
    printf("Your UID is %d\n", getuid());
}

int main() {
    char input_buffer[MAX_LINE];
    char* args[MAX_NUM_ARGS];
    char* args_pipe[MAX_NUM_ARGS];
    int should_run = 1;
    int background = 0;
    int pipe = 0;

    printf("Welcome to 1104526shell\n");

    while (should_run) {
        printf("1104526shell> ");
        fgets(input_buffer, MAX_LINE, stdin);
        input_buffer[strcspn(input_buffer, "\n")] = 0;

        if (strcmp(input_buffer, "exit") == 0) {
            should_run = 0;
            continue;
        }

        char* next = strchr(input_buffer, '&');
        if (next) {
            background = 1;
            *next = ' ';
        } else {
            background = 0;
        }

        next = strchr(input_buffer, '|');
        if (next) {
            pipe = 1;
            *next = '\0';
            string_parser(input_buffer, args);
            string_parser(next + 1, args_pipe);
        } else {
            pipe = 0;
            string_parser(input_buffer, args);
        }

        int out = -1;
        int stdout_copy = dup(STDOUT_FILENO);
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], ">") == 0) {
                args[i] = NULL;
                out = open(args[i + 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                break;
            }
        }

        if (out != -1) {
            dup2(out, STDOUT_FILENO);
            close(out);
        }

        if (strcmp(args[0], "s1104526") == 0) {
            execute_s1104526();
            dup2(stdout_copy, STDOUT_FILENO);
            continue;
        }

        if (pipe) {
            execute_pipe(args, args_pipe);
        } else {
            execute_command(args, background);
        }

        dup2(stdout_copy, STDOUT_FILENO);
    }

    return 0;
}