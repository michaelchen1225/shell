#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80
#define MAX_NUM_ARGS 10

size_t string_parser(char *input, char *word_array[]) {
    size_t n = 0;
    while (*input) {
        while (isspace((unsigned char)*input))
            ++input;
        if (*input) {
            word_array[n++] = (char *)input;
            while (*input && !isspace((unsigned char)*input))
                ++input;
            *(input) = '\0';
            ++input;
        }
    }
    word_array[n] = NULL;
    return n;
}

void execute_command(char **args, int background) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("Error");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Error");
    } else {
        if (!background)
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

void redirect(char **args, char *input_file, char *output_file, int option) {
    pid_t pid;
    int file_descriptor; 

    pid = fork();
    if (pid == 0) {
        if (option == 0) {
            file_descriptor = open(output_file, O_CREAT | O_TRUNC | O_WRONLY, 0600); 
            dup2(file_descriptor, STDOUT_FILENO); 
            close(file_descriptor);
        } else if (option == 1) {
            file_descriptor = open(input_file, O_RDONLY, 0600);  
            dup2(file_descriptor, STDIN_FILENO); 
            close(file_descriptor);
        }
        if (execvp(args[0], args) == -1) {
            perror("Error");
        }
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

void pipe_command(char **args, char **args_pipe) {
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

int main() {
    char input_string[MAX_LINE]; 
    char *tokens[MAX_NUM_ARGS];
    char *input_file = NULL;
    char *output_file = NULL;
    char *args[MAX_NUM_ARGS];
    char *args_pipe[MAX_NUM_ARGS];
    int numTokens;
    int background = 0;
    int redirect_in = 0;
    int redirect_out = 0;
    int pipe_flag = 0;

    while (1) {
        background = 0;
        printf("s1104526shell> ");
        gets(input_string);
        numTokens = string_parser(input_string, tokens);

        if (strcmp(tokens[0], "exit") == 0)
            break;

        for (int i = 0; i < numTokens; i++) {
            if (strcmp(tokens[i], ">") == 0) {
                output_file = tokens[i + 1];
                redirect_out = 1;
                tokens[i] = NULL;
            }

            if (strcmp(tokens[i], "<") == 0) {
                input_file = tokens[i + 1];
                redirect_in = 1;
                tokens[i] = NULL;
            }

            if (strcmp(tokens[i], "|") == 0) {
                tokens[i] = NULL;
                pipe_flag = 1;
                int j = 0;
                for (j = i + 1; j < numTokens; j++) {
                    args_pipe[j - i - 1] = tokens[j];
                }
                args_pipe[j - i - 1] = NULL;
            }

            if (strcmp(tokens[i], "&") == 0) {
                background = 1;
                tokens[i] = NULL;
            }
        }

        for (int i = 0; i < numTokens; i++) {
            args[i] = tokens[i];
        }

        if (strcmp(args[0], "s1104526") == 0) {
            printf("your uid is %d\n", getuid());
            continue;
        }

        if (pipe_flag) {
            pipe_command(args, args_pipe);
            continue;
        }

        if (redirect_out) {
            redirect(args, NULL, output_file, 0);
            continue;
        }

        if (redirect_in) {
            redirect(args, input_file, NULL, 1);
            continue;
        }

        execute_command(args, background);
    }

    return 0;
}