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
    int error = 0;
    int file_fd = -1;
    int data_fd = -1;
    SharedData *data = MAP_FAILED;
    sem_t *sem_ready = SEM_FAILED;
    sem_t *sem_read = SEM_FAILED;
    pid_t pid = -1;

    char file_name[PATH_MAX];
    printf("Enter a file name\n");
    
    if (fgets(file_name, sizeof(file_name), stdin) == NULL)
    {
        fprintf(stderr, "Invalid input\n");
        return 1;
    }
    file_name[strcspn(file_name, "\n")] = '\0';

   
    file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1)
    {
        perror("open");
        error = 1;
        goto cleanup;
    }

    data_fd = shm_open("/my_shared", O_CREAT | O_RDWR | O_EXCL, 0600);
    if (data_fd == -1)
    {
        perror("shm_open");
        error = 1;
        goto cleanup;
    }

    if (ftruncate(data_fd, sizeof(SharedData)) == -1)
    {
        perror("ftruncate");
        error = 1;
        goto cleanup;
    }

    data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
    if (data == MAP_FAILED)
    {
        perror("mmap");
        error = 1;
        goto cleanup;
    }
    data->result = 0;
    data->finished = 0;

    sem_ready = sem_open("/sem_ready", O_CREAT | O_EXCL, 0600, 0);
    if (sem_ready == SEM_FAILED)
    {
        perror("sem_open ready");
        error = 1;
        goto cleanup;
    }

    sem_read = sem_open("/sem_read", O_CREAT | O_EXCL, 0600, 0);
    if (sem_read == SEM_FAILED)
    {
        perror("sem_open read");
        error = 1;
        goto cleanup;
    }


    
    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        error = 1;
        goto cleanup;
    }
    else if (pid == 0)
    {
        printf("Child PID: %d\n", getpid());
        fflush(stdout);

        if (dup2(file_fd, STDIN_FILENO) == -1)
        {
            perror("dup2 stdin");
            return 1;
        }

        if (close(file_fd) == -1)
        {
            perror("close file_fd");
            return 1;
        }
        if (close(data_fd) == -1)
        {
            perror("close data_fd");
            return 1;
        }
        execl("./child", "child", (char *)NULL);
        perror("execl");
        return 1;
    }

    {
        if (close(file_fd) == -1)
        {
            perror("close file_fd");
            error = 1;
            goto cleanup;
        }
        file_fd = -1;
        while (1)
        {
            if (sem_wait(sem_ready) == -1)
            {
                perror("sem_wait ready");
                error = 1;
                break;
            }
            if(data->finished == 1){
                break;
            }
            

            printf("%g\n", data->result);
            if (sem_post(sem_read) == -1)
            {
                perror("sem_post read");
                error = 1;
                break;
            }
        }
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid");
            error = 1;
            goto cleanup;
        }
    
        if (WIFEXITED(status))
        {
            printf("Child exit code: %d\n", WEXITSTATUS(status));

            if (WEXITSTATUS(status) != 0)
            {
                error = 1;
            }
        }
        else if (WIFSIGNALED(status))
        {
            printf("Child killed by signal %d\n", WTERMSIG(status));
            error = 1;
        }
    }

    cleanup:

    if (sem_read != SEM_FAILED)
    {
        if (sem_close(sem_read) == -1)
        {
            perror("sem_close read");
            error = 1;
        }
    }

    if (sem_ready != SEM_FAILED)
    {
        if (sem_close(sem_ready) == -1)
        {
            perror("sem_close ready");
            error = 1;
        }
    }

    if (data != MAP_FAILED)
    {
        if (munmap(data, sizeof(SharedData)) == -1)
        {
            perror("munmap");
            error = 1;
        }
    }

    if (data_fd != -1)
    {
        if (close(data_fd) == -1)
        {
            perror("close data_fd");
            error = 1;
        }
    }

    if (file_fd != -1)
    {
        if (close(file_fd) == -1)
        {
            perror("close file_fd");
            error = 1;
        }
    }

    if (sem_ready != SEM_FAILED)
    {
        if (sem_unlink("/sem_ready") == -1)
        {
            perror("sem_unlink ready");
            error = 1;
        }
    }
    if (sem_read != SEM_FAILED){
        if (sem_unlink("/sem_read") == -1)
        {
            perror("sem_unlink read");
            error = 1;
        }
    }
    if(data_fd != -1){
        if (shm_unlink("/my_shared") == -1)
        {
            perror("shm_unlink");
            error = 1;
        }
    }
    

    return error;

}