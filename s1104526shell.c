#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */

void parse_args(char *line, char **argv) {
    while (*line != '\0') {
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0';
        *argv++ = line;
        while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n') 
            line++;
    }
    *argv = '\0';
}

void execute(char **argv) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        printf("Fork error\n");
        exit(1);
    }
    else if (pid == 0) {
        if (execvp(*argv, argv) < 0) {
            printf("Execvp error\n");
            exit(1);
        }
    }
    else {
        while (wait(&status) != pid);
    }
}

void execute_with_pipe(char **argv, char **argv2) {
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        printf("Pipe error\n");
        exit(1);
    }
    p1 = fork();
    if (p1 < 0) {
        printf("Fork error\n");
        exit(1);
    }

    if (p1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(argv[0], argv) < 0) {
            printf("Execvp error\n");
            exit(1);
        }
    } else {
        p2 = fork();

        if (p2 < 0) {
            printf("Fork error\n");
            exit(1);
        }

        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            if (execvp(argv2[0], argv2) < 0) {
                printf("Execvp error\n");
                exit(1);
            }
        } else {
            wait(NULL);
            wait(NULL);
        }
    }
}

void execute_with_redirection(char **argv, char *file, int mode) {
    pid_t pid;
    int fd;

    if ((pid = fork()) < 0) {
        printf("Fork error\n");
        exit(1);
    }
    else if (pid == 0) {
        if (mode == 0) {
            fd = open(file, O_RDONLY);
            dup2(fd, STDIN_FILENO);
        } else {
            fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            dup2(fd, STDOUT_FILENO);
        }
        close(fd);
        if (execvp(argv[0], argv) < 0) {
            printf("Execvp error\n");
            exit(1);
        }
    }
    else {
        wait(NULL);
    }
}

void execute_in_background(char **argv) {
    pid_t pid;

    if ((pid = fork()) < 0) {
        printf("Fork error\n");
        exit(1);
    }
    else if (pid == 0) {
        if (execvp(argv[0], argv) < 0) {
            printf("Execvp error\n");
            exit(1);
        }
    }
}

void execute_special_command() {
    printf("Your UID is %d\n", getuid());
}

int main(void) {
    char line[MAX_LINE];
    char *argv[MAX_LINE/2 + 1];
    char *argv2[MAX_LINE/2 + 1];
    char *file;
    int mode;

    while (1) {
        printf("Shell> ");
        fgets(line, sizeof(line), stdin);
        parse_args(line, argv);
        if (strcmp(argv[0], "exit") == 0) 
            exit(0);
        else if (strcmp(argv[0], "s1104526") == 0)
            execute_special_command();
        else if (strchr(line, '|') != NULL) {
            parse_args(strchr(line, '|') + 1, argv2);
            execute_with_pipe(argv, argv2);
        } else if (strchr(line, '>') != NULL) {
            file = strchr(line, '>') + 1;
            mode = 1;
            execute_with_redirection(argv, file, mode);
        } else if (strchr(line, '<') != NULL) {
            file = strchr(line, '<') + 1;
            mode = 0;
            execute_with_redirection(argv, file, mode);
        } else if (strchr(line, '&') != NULL) {
            execute_in_background(argv);
        } else {
            execute(argv);
        }
    }

    return 0;
}