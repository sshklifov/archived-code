#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

/* This function will always return NULL on failure. 
 *
 * It will not set errno, except for EINVAL (argument check) and
 * ECHILD (child failed). The idea behind this behaviour is to follow
 * the normal flow control of sys calls made -- pipe, fork, close
 * and dup2, waitpid. It is worth mentioning that the forked child does
 * not perform error checking and returnes whatever the programme 'cmd'
 * returns. If exec failed, then the predefined value of 127 is returned.
 */

#define __sys_assert(x) if (x == -1) return NULL;

/* Macro for lazy error checking which conforms to the written above */

FILE* __popen (const char* cmd, const char* mode)
{
    if (strcmp (mode, "r") != 0 && strcmp (mode, "w") != 0 )
    {
        errno = EINVAL;
        return NULL;
    }

    int pipefd[2];
    __sys_assert(pipe (pipefd));

    int p_fd, c_fd, reopen_fd;
    if (strcmp (mode, "r") == 0)
    {
        p_fd = pipefd[0];
        c_fd = pipefd[1];
        reopen_fd = STDOUT_FILENO;
    }
    else
    {
        p_fd = pipefd[1];
        c_fd = pipefd[0];
        reopen_fd = STDIN_FILENO;
    }

    int chld_pid;
    switch ((chld_pid = fork ()))
    {
        case -1:
        {
            return NULL;
        }
        case 0:
        {
            close (p_fd);
            dup2 (c_fd, reopen_fd);
            close (c_fd);

            execl ("/bin/sh", "/bin/sh", "-c", cmd);
            _exit (127);
        }
    }

    __sys_assert (close (c_fd));

    int wstatus;
    __sys_assert (waitpid (chld_pid, &wstatus, 0));

    if (WEXITSTATUS (wstatus) != 0)
    {
        errno = ECHILD;
        return NULL;
    }

    FILE* fp = fdopen (p_fd, mode);
    if (fp == NULL) return NULL;

    errno = 0;
    return fp;
}

int __pclose (FILE* fp)
{
    return fclose (fp);
}

int main ()
{
    FILE* fp = __popen ("sleep 3; echo hi", "r");
    if (fp == NULL)
    {
        perror ("popen");
        exit (EXIT_FAILURE);
    }

    char buf[4096];
    int s;

    do
    {
        int s = fread (buf, 1, 4096, fp);
        
        if (ferror (fp))
        {
            perror ("fread");
            exit (EXIT_FAILURE);
        }

        printf ("read %d bytes: %s\n", s, buf);
    }
    while (!feof (fp));

    printf ("eof reached, exiting...\n");
}
