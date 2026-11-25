
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdlib.h>
#include "shared.h"

int main(void)
{
    char *line = NULL;
    size_t cap = 0;
    int error = 0;

    int data_fd = -1;
    SharedData *data = MAP_FAILED;
    sem_t *sem_ready = SEM_FAILED;
    sem_t *sem_read = SEM_FAILED;

    data_fd = shm_open("/my_shared", O_RDWR, 0600);
    if (data_fd == -1)
    {
        perror("shm_open");
        error = 1;
        goto cleanup;
    }

    data = mmap(NULL, sizeof(SharedData),PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
    if (data == MAP_FAILED)
    {
        perror("mmap");
        error = 1;
        goto cleanup;
    }

    sem_ready = sem_open("/sem_ready", 0);
    if (sem_ready == SEM_FAILED)
    {
        perror("sem_open ready");
        error = 1;
        goto cleanup;
    }

    sem_read = sem_open("/sem_read", 0);
    if (sem_read == SEM_FAILED)
    {
        perror("sem_open read");
        error = 1;
        goto cleanup;
    }

    while (getline(&line, &cap, stdin) != -1)
    {
        char *ptr = line;
        char *end;
        float num;
        float sum = 0;

        while (1)
        {
            num = strtof(ptr, &end);
            if (end == ptr)
            {
                break;
            }
            sum += num;
            ptr = end;
        }

        while (*ptr == ' ' || *ptr == '\n' || *ptr == '\t')
        {
            ptr++;
        }

        if (*ptr == '\0')
        {
            data->result = sum;
            data->finished = 0;

            if (sem_post(sem_ready) == -1)
            {
                perror("sem_post");
                error = 1;
                goto cleanup;
            }

            if (sem_wait(sem_read) == -1)
            {
                perror("sem_wait");
                error = 1;
                goto cleanup;
            }
        }
        else
        {
            fprintf(stderr, "Error in parsing\n");
        }
    }

    if (ferror(stdin))
    {
        perror("getline");
        error = 1;
    }

    data->finished = 1;

    if (sem_post(sem_ready) == -1)
    {
        perror("sem_post");
        error = 1;
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
            perror("close");
            error = 1;
        }
    }

    free(line);
    return error;
}
