#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>

int main (int argc, char** argv)
{
    if (argc == 1)
    {
        fprintf (stderr, "Specify command to run\n");
        exit (EXIT_FAILURE);
    }

    switch (fork ())
    {
        case -1:
        {
            perror ("errno");
            exit (EXIT_FAILURE);
        }

        case 0:
        {
            execvp (argv[1], argv + 1);
            _exit (EXIT_FAILURE);
        }

        default:
        {
            if (wait (NULL) == -1)
            {
                perror ("waot");
                exit (EXIT_FAILURE);
            }

            struct rusage usage;

            if (getrusage (RUSAGE_CHILDREN, &usage) == -1)
            {
                perror ("getrusage");
                exit (EXIT_FAILURE);
            }

            printf ("u=%d.%ds s=%d.%ds\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec, usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
        }
    }

    exit (EXIT_SUCCESS);
}
