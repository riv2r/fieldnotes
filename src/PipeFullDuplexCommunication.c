#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char const *argv[])
{
    /*
     fd0 child<-parent
     fd1 child->parent
     */
    int fd0[2]; // child read, parent write
    int fd1[2]; // child write, parent read
    if (pipe(fd0) == -1 || pipe(fd1) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        close(fd0[1]);
        close(fd1[0]);
        dup2(fd0[0], STDIN_FILENO);
        dup2(fd1[1], STDOUT_FILENO);
        close(fd0[0]);
        close(fd1[1]);
        char buf[100];
        scanf(" %[^\n]", buf);
        fprintf(stderr, "Child received: %s\n", buf);

        printf("Hello from child\n");
        fflush(stdout);
    }
    else
    {
        close(fd0[0]);
        close(fd1[1]);
        dup2(fd0[1], STDOUT_FILENO);
        dup2(fd1[0], STDIN_FILENO);
        close(fd0[1]);
        close(fd1[0]);
        printf("Hello from parent\n");
        fflush(stdout);

        char buf[100];
        scanf(" %[^\n]", buf);
        fprintf(stderr, "Parent received: %s\n", buf);

        wait(NULL);
    }
    return 0;
}
