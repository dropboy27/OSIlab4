#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include "shared.h"

int main(void)
{
    char file_name[PATH_MAX];
    printf("Enter a file name\n");
    fflush(stdout);
    
    if (fgets(file_name, sizeof(file_name), stdin) == NULL)
    {
        fprintf(stderr, "Invalid input\n");
        return 1;
    }
    file_name[strcspn(file_name, "\n")] = '\0';
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1)
    {
        perror("open");
        return 1;
    }
    int data_fd = shm_open("/my_shared", O_CREAT | O_RDWR, 0600);
    if(data_fd == -1){
        perror("shm error");
        return 1;
    }
    if (ftruncate(data_fd, sizeof(SharedData)) == -1)
    {
        perror("ftruncate");
        close(data_fd);
        return 1;
    }
    float *data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
    if (data == MAP_FAILED)
    {
        perror("mmap");
        close(data_fd);
        return 1;
    }
    sem_t *sem_ready = sem_open("/sem_ready", O_CREAT | O_EXCL, 0600, 0);
    if(sem_ready == SEM_FAILED){
        perror("sem_ready error");
        close(data_fd);
        return 1;
    }
    sem_t *sem_read = sem_open("/sem_read", O_CREAT | O_EXCL, 0600, 0);
    if(sem_read == SEM_FAILED){
        perror("sem_read error");
        close(data_fd);
        return 1;
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return 1;
    }
    else if (pid == 0)
    {
        printf("Child PID: %d\n", getpid());
        fflush(stdout);
        dup2(file_fd, 0);
        close(file_fd);
        execl("./child", "child", (char *)NULL);
        perror("execl");
        return 1;
    }
    else
    {
        sem_wait(sem_ready)

        close(file_fd);
        char buf[256];
        ssize_t n;
        while ((n = read(pipe1[0], buf, 256)) > 0)
        {
            write(1, buf, n);
        }
        if (n == -1)
        {
            perror("read");
        }
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid");
            return 1;
        }
        if (WIFEXITED(status))
        {
            printf("Child exit code: %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Child killed by signal %d\n", WTERMSIG(status));
        }
    }
    return 0;
}