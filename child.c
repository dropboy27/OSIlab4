#include <stdio.h>
#include <stdlib.h>

int main()
{
    setbuf(stdout, NULL);
    char *line = NULL;
    size_t cap = 0;
    while (getline(&line, &cap, stdin) != -1)
    {
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