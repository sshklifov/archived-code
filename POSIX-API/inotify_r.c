#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ftw.h>
#include <sys/inotify.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

int fd;

int per_dir_fun (const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf)
{
    if (typeflag != FTW_D) return 0;

    printf ("Putting directory %s on watchlist...\n", fpath);

    if (inotify_add_watch (fd, fpath, IN_CREATE | IN_DELETE | IN_MOVE | IN_ONLYDIR) == -1)
    //if (inotify_add_watch (fd, fpath, IN_ALL_EVENTS) == -1)
    {
        perror ("inotify_add_watch");
        fprintf (stderr, "Warning. Could not add file \'%s\' to watch list.\n", fpath);
    }

    return 0;
}

void print_event (struct inotify_event* event)
{
    char type[256];

    switch (event->mask)
    {
        case IN_CREATE: strcpy(type, "file created"); break;
        
        case IN_DELETE: strcpy(type, "file deleted"); break;
       
        case IN_MOVED_FROM: strcpy(type, "moved from dir"); break;
       
        case IN_MOVED_TO: strcpy(type, "file to dir"); break;

        default: strcpy (type, "unknown event");
    }

    printf ("%s: %s%s\n", event->name, type, event->cookie == 0 ? " (cookie set)" : "");
}

int main ()
{
    fd = inotify_init();

    if (fd == -1)
    {
        perror ("inotify_init");
        exit (EXIT_FAILURE);
    }

    if (nftw (".", per_dir_fun, 50, FTW_PHYS | FTW_MOUNT) == -1)
    {
        fprintf (stderr, "\'nftw\' failed. Too sad\n");
    }

    errno = 0;
    long pc_name_max = pathconf (".", _PC_NAME_MAX);

    if (pc_name_max == -1)
    {
        if (errno) perror ("pathconf");
        else fprintf (stderr, "_PC_NAME_MAX is not defined!\n");

        exit (EXIT_FAILURE);
    }

    size_t bufsiz = 10 * (sizeof (struct inotify_event) + pc_name_max + 1);
    void * buf = malloc (bufsiz);

    while (1)
    {
        ssize_t bytes_read = read (fd, buf, bufsiz);
        if (bytes_read == -1)
        {
            perror ("read");
            exit (EXIT_FAILURE);
        }

        for (void* p = buf; p < buf + bytes_read;)
        {
            struct inotify_event* event = (struct inotify_event*)p;
            print_event (event);

            p += sizeof (struct inotify_event) + event->len;
        }
    }
    
    free (buf);
    close (fd);
}
