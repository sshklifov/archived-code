#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

enum tstate
{
    THREAD_RUNNING,
    THREAD_ZOMBIE,
    THREAD_REAPED
};

typedef struct thread
{
    pthread_t id;
    int sleep;
    enum tstate status;
} thread;

thread* arr;

pthread_cond_t cond;
pthread_mutex_t mutex;

int count;
int numUnreaped;

void* func (void* infop)
{
    sleep ((long)infop);

    pthread_t self = pthread_self();

    for (int i = 0; i < count; i++)
    {
        if (arr[i].id == self)
        {
            arr[i].status = THREAD_ZOMBIE;
            break;
        }
    }

    int s;

    s = pthread_mutex_lock (&mutex);
    if (s != 0)
    {
        perror ("pthread_mutex_lock");
        exit (EXIT_FAILURE);
    }

    numUnreaped++;

    s = pthread_mutex_unlock (&mutex);
    if (s != 0)
    {
        perror ("pthread_mutex_unlock");
        exit (EXIT_FAILURE);
    }

    s = pthread_cond_signal (&cond);
    if (s != 0)
    {
        perror ("pthread_cond_signal");
        exit (EXIT_FAILURE);
    }
}

int main (int argc, char** argv)
{
    if (argc == 1)
    {
        // fprintf (stderr, "%s\n", strerror(EINVAL) );
        fprintf (stderr, "usage: ./cond_var [seconds to sleep thread]...\n");
        exit (EXIT_FAILURE);
    }

    int s;

    s = pthread_mutex_init (&mutex, NULL);
    if (s != 0)
    {
        // Cannot fail
        fprintf (stderr, "Critical: \'pthread_mutex_init\' failed.\n");
    }

    s = pthread_cond_init (&cond, NULL);
    if (s != 0)
    {
        // Cannot fail
        fprintf (stderr, "Critical: \'pthread_cond_init\' failed.\n");
    }

    count = argc - 1;
    arr = malloc (sizeof (thread) * count);

    if (arr == NULL)
    {
        perror ("malloc");
        exit (EXIT_FAILURE);
    }
    
    numUnreaped = 0;
    for (int i = 0; i < count; i++)
    {
        int savedErrno = errno;
        errno = 0;

        long secs = strtol (argv[i + 1], NULL, 10);

        if (errno == ERANGE)
        {
            perror ("strtol");
            exit (EXIT_FAILURE);
        }

        errno = savedErrno;

        if (secs == 0L)
        {
            fprintf (stderr, "%s\n", strerror(EINVAL) );
            exit (EXIT_FAILURE);
        }

        pthread_t tid;

        s = pthread_create (&tid, NULL, func, (void*)secs);
        if (s != 0)
        {
            perror ("pthread_create");
            exit (EXIT_FAILURE);
        }

        arr[i] = (thread){tid, secs, THREAD_RUNNING};

        printf ("started thread %ld, sleep time is %ld,\n", tid, secs);
    }

    int numReaped = 0;
    while (numReaped != count)
    {
        s = pthread_mutex_lock (&mutex);
        if (s != 0)
        {
            perror ("pthread_mutex_lock");
            exit (EXIT_FAILURE);
        }

        while (numUnreaped == 0)
        {
            s = pthread_cond_wait (&cond, &mutex);
            if (s != 0)
            {
                perror ("pthread_mutex_lock");
                exit (EXIT_FAILURE);
            }
        }

        for (int i = 0; i < count; i++)
        {
            if (arr[i].status == THREAD_ZOMBIE)
            {
                printf ("Caught a terminated thread with id %ld, sleep time was %d.\n", arr[i].id, arr[i].sleep);
                arr[i].status = THREAD_REAPED;
                numReaped++;
            }
        }

        s = pthread_mutex_unlock (&mutex);
    }

    s = pthread_cond_destroy (&cond);
    if (s != 0)
    {
        perror ("pthread_cond_destroy");
        exit (EXIT_FAILURE);
    }

    s = pthread_mutex_destroy (&mutex);
    if (s != 0)
    {
        perror ("pthread_mutex_destroy");
        exit (EXIT_FAILURE);
    }

    exit (EXIT_SUCCESS);
}
