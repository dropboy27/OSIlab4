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
int main()
{
    setbuf(stdout, NULL);
    char *line = NULL;
    size_t cap = 0;
    while (getline(&line, &cap, stdin) != -1)
    {
        int data_fd = shm_open("/my_shared", O_RDWR,0600);
        if(data_fd == -1){
            perror("shm error");
            return 1;
        }
        float *data = mmap(NULL, sizeof(SharedData),PROT_READ | PROT_WRITE, MAP_SHARED,data_fd,0);
        if (data == MAP_FAILED)
        {
            perror("mmap");
            close(data_fd);
            return 1;
        }
        sem_t *sem_ready = sem_open("/sem_ready", 0);
        sem_t *sem_read = sem_open("/sem_read", 0);
        char *ptr = line;
        char *end;
        float num;
        float sum = 0;
        int error = 0;

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
            printf("%g\n", sum);
        }
        else
        {
            fprintf(stderr, "Error in parsing\n");
        }
    }
    free(line);
    return 0;
}