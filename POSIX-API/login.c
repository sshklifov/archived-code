#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <pwd.h>
#include <sys/capability.h>
#include <shadow.h>
#include <crypt.h>
#include <fcntl.h>
#include <paths.h>
#include <lastlog.h>
#include <time.h>
#include <utmpx.h>
#include <stdint.h>
#include <sys/wait.h>

/* TODO Probably substitute calls to write with calls to fread */
/* TODO Formatting */

void disable_core_dump ()
{
    struct rlimit* old_lim = malloc (sizeof (struct rlimit));

    if (old_lim == NULL)
    {
        syslog (LOG_ERR, "call to malloc(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (getrlimit (RLIMIT_STACK, old_lim) == -1)
    {
        syslog (LOG_ERR, "call to getrlimit(2) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    struct rlimit new_lim;
    new_lim.rlim_cur = 0;
    new_lim.rlim_max = old_lim->rlim_max;

    if (setrlimit (RLIMIT_CORE, &new_lim) == -1)
    {
        syslog (LOG_ERR, "call to setrlimit(2) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }
}

void cap_set_dac_override ()
{
    cap_t cap = cap_get_proc ();
    if (cap == NULL)
    {
        syslog (LOG_ERR, "call to cap_get_proc(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    cap_value_t dac_override = CAP_DAC_OVERRIDE;
    if (cap_set_flag (cap, CAP_EFFECTIVE, 1, &dac_override, CAP_SET) == -1)
    {
        syslog (LOG_ERR, "call to cap_set_flag(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);

    }

    if (cap_set_proc (cap) == -1)
    {
        perror ("cap_set_proc");
        syslog (LOG_ERR, "call to cap_set_proc(CAP_DAC_OVERRIDE, CAP_SET) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (cap_free (cap) == -1)
    {
        syslog (LOG_ERR, "call to cap_free(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }
}

void cap_drop_dac_override ()
{
    cap_t cap = cap_get_proc ();
    if (cap == NULL)
    {
        syslog (LOG_ERR, "call to cap_get_proc(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    cap_value_t dac_override = CAP_DAC_OVERRIDE;
    if (cap_set_flag (cap, CAP_EFFECTIVE, 1, &dac_override, CAP_CLEAR) == -1)
    {
        syslog (LOG_ERR, "call to cap_set_flag(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);

    }

    if (cap_set_proc (cap) == -1)
    {
        perror ("cap_set_proc");
        syslog (LOG_ERR, "call to cap_set_proc(CAP_DAC_OVERRIDE, CAP_CLEAR) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (cap_free (cap) == -1)
    {
        syslog (LOG_ERR, "call to cap_free(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

}

void cap_drop_all ()
{
    cap_t empty_cap = cap_init();

    if (empty_cap == NULL)
    {
        syslog (LOG_ERR, "call to cap_init(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (cap_set_proc (empty_cap) == -1)
    {
        syslog (LOG_ERR, "call to cap_set_proc(empty_cap) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (cap_free (empty_cap) == -1)
    {
        syslog (LOG_ERR, "call to cap_free(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }
}

char* get_username ()
{
    const char* user_prompt = "Username: ";

    printf ("Username: ");
    if (ferror (stdout))
    {
        syslog (LOG_ERR, "call to printf(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    long name_len = sysconf (_SC_LOGIN_NAME_MAX);
    if (name_len == -1)
    {
        syslog (LOG_ERR, "call to sysconf(_SC_LOGIN_NAME_MAX) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    char* user_name = malloc (name_len + 1); // plus delimiter
    if (user_name == NULL)
    {
        syslog (LOG_ERR, "call to malloc(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    fgets (user_name, name_len + 1, stdin);
    if (ferror (stdin))
    {
        syslog (LOG_ERR, "call to fgets(username) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    size_t bytes = strlen (user_name);
    void* p = realloc (user_name, bytes); // if failed, memory is left untouched
    if (p != NULL)
    {
        user_name = p;
    }

    else
    {
        syslog(LOG_WARNING, "tried to shrink malloc-ed memory but call failed: %s", strerror (errno));
    }

    user_name[bytes - 1] = '\0';

    return user_name;
}

struct passwd* get_passwd (char* username)
{
    errno = 0;
    struct passwd* pwd = getpwnam (username);

    if (pwd == NULL)
    {
        if (errno)
        {
            syslog (LOG_ERR, "call to getpwnam(3) failed: %s", strerror (errno));
            exit (EXIT_FAILURE);
        }

        else
        {
            /* TODO Do not error out */
            syslog (LOG_ERR, "user \'%s\' was not found on the system", username);
            exit (EXIT_FAILURE);
        }
    }

    free (username);
    return pwd;
}

char* get_shadow (struct passwd* pwd)
{
    cap_set_dac_override ();
    struct spwd* spwd = getspnam (pwd->pw_name); // call shall not fail
    cap_drop_dac_override ();

    return spwd == NULL ? pwd->pw_passwd : spwd->sp_pwdp;
}

int verify_user (char* passwd)
{
    char* entered_passwd = getpass("Password: ");
    if (entered_passwd == NULL)
    {
        syslog (LOG_ERR, "call to getpass(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    char* encrypted_passwd = crypt(entered_passwd, passwd);
    if (encrypted_passwd == NULL)
    {
        syslog (LOG_ERR, "call to crypt(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    for (char* p = entered_passwd; *p != '\0';)
    {
        *p = '\0';
        p++;
    }

    return strcmp (passwd, encrypted_passwd) == 0 ? 1 : 0;
}

void write_to_lastlog (uid_t uid)
{
    cap_set_dac_override ();

    int fd = open (_PATH_LASTLOG, O_RDWR);

    if (fd == -1)
    {
        syslog (LOG_ERR, "call to open(_PATH_LASTLOG) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }
    
    if (lseek (fd, uid * sizeof (struct lastlog), SEEK_SET) == -1)
    {
        syslog (LOG_ERR, "call to lseek(_PATH_LASTLOG) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    struct lastlog ll;

    if (read (fd, &ll, sizeof (struct lastlog)) == -1)
    {
        syslog (LOG_ERR, "call to read(_PATH_LASTLOG) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    time_t last_log_time = ll.ll_time;
    if (printf ("Last login: %son %s\n\n", ctime (&last_log_time), ll.ll_line) < 0)
    {
        syslog (LOG_ERR, "call to printf(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    time_t now = time (NULL);
    if (now > INT32_MAX)
    {
        syslog (LOG_WARNING, "time overflow; will allow it");
    }
    ll.ll_time = (int32_t)now;

    char* term_device = ttyname (STDIN_FILENO);
    if (term_device == NULL)
    {
        syslog (LOG_ERR, "call to ttyname(STDIN_FILENO) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }
    strcpy(ll.ll_line, term_device);

    memset (ll.ll_host, 0, UT_HOSTSIZE); // do not set a hostname by default

    if (lseek (fd, uid * sizeof (struct lastlog), SEEK_SET) == -1)
    {
        syslog (LOG_ERR, "call to lseek(_PATH_LASTLOG) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (write (fd, &ll, sizeof (struct lastlog)) == -1)
    {
        syslog (LOG_ERR, "call to write(_PATH_LASTLOG) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    cap_drop_dac_override ();
}

void write_to_utmp_wtmp (char* username)
{
    struct utmpx ut;
    memset (&ut, 0, sizeof (struct utmpx));

    ut.ut_type = USER_PROCESS;
    ut.ut_pid = getpid ();
    
    char* term_device = ttyname (STDIN_FILENO);
    if (term_device == NULL)
    {
        syslog (LOG_ERR, "call to ttyname(STDIN_FILENO) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    strncpy(ut.ut_line, term_device + strlen ("/dev/"), sizeof (ut.ut_line));
    strncpy(ut.ut_id, term_device + strlen ("/dev/***"), sizeof (ut.ut_id));
    
    strcpy (ut.ut_user, username);
    
    time_t now;

    if(time (NULL) == -1)
    {
        syslog (LOG_ERR, "call to time(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (now > INT32_MAX)
    {
        syslog (LOG_WARNING, "time overflow; will allow it");
    }

    ut.ut_tv.tv_sec = (int32_t)now;

    cap_set_dac_override ();

    setutxent ();
    if (pututxline (&ut) == NULL)
    {
        syslog (LOG_ERR, "call to pututxline(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    updwtmpx (_PATH_WTMP, &ut);
    endutent ();

    cap_drop_dac_override ();
}

void write_to_utmp_wtpm_atexit ()
{
    struct utmpx ut;
    memset (&ut, 0, sizeof (struct utmpx));

    ut.ut_type = DEAD_PROCESS;
    strncpy (ut.ut_id, ttyname (STDIN_FILENO) + strlen ("/dev/***"), sizeof (ut.ut_id));

    setutxent ();

    cap_set_dac_override ();
    if (pututxline (&ut) == NULL)
    {
        syslog (LOG_ERR, "call to pututxline(3) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }
    updwtmpx (_PATH_WTMP, &ut);
    cap_drop_dac_override ();

    endutent ();
}

int main ()
{
    openlog(NULL, 0, LOG_USER);
    disable_core_dump ();

    int attempts = 0;
    struct passwd* user;

    do
    {
        attempts++;

        user = get_passwd (get_username ());
        if (verify_user (get_shadow (user)) != 0)
        {
            break;
        }

        const char* retry_msg = "Login incorrect (%d attempts total)\n\n";
        if (printf ("Login incorrect (%d attempts total)\n\n", attempts) < 0)
        {
            syslog (LOG_ERR, "call to printf(3) failed: %s", strerror (errno));
            exit (EXIT_FAILURE);
        }
    }
    while (attempts < 4);

    if (attempts == 4)
    {
        printf ("Maximum number of authentication attempts reached.\n");
        if (ferror (stdout))
        {
            syslog (LOG_ERR, "call to printf(3) failed: %s", strerror (errno));
            exit (EXIT_FAILURE);
        }

        exit (EXIT_FAILURE);
    }

    /* TODO Check for expiration date */

    write_to_lastlog (user->pw_uid);
    write_to_utmp_wtmp(user->pw_name);
    
    if (setuid (user->pw_uid) == -1)
    {
        syslog (LOG_ERR, "call to setuid(user) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (chdir (user->pw_dir) == -1)
    {
        syslog (LOG_ERR, "call to chdir(HOME) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (setgid (user->pw_gid) == -1)
    {
        syslog (LOG_ERR, "call to setgid(group) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (setenv("HOME", user->pw_dir, 0) == -1)
    {
        syslog (LOG_ERR, "call to setenv(HOME) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (setenv("USER", user->pw_name, 0) == -1)
    {
        syslog (LOG_ERR, "call to setenv(USER) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (setenv("SHELL", user->pw_shell, 0) == -1)
    {
        syslog (LOG_ERR, "call to setenv(SHELL) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    const char* pathenv = user->pw_uid == 0 ?
        "/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin" :
        "/usr/local/bin:/bin:/usr/bin";

    if (setenv("PATH", pathenv, 0) == -1)
    {
        syslog (LOG_ERR, "call to setenv(PATH) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }

    /* TODO Set remaining environmental variables */

    switch (fork ())
    {
        case -1:
        {
            syslog (LOG_ERR, "call to fork(2) failed: %s", strerror (errno));
            exit (EXIT_FAILURE);
        }

        case 0:
        {
            
            execlp (user->pw_shell, user->pw_shell, NULL);

            syslog (LOG_ERR, "call to execlp(shell) failed: %s", strerror (errno));
            exit (EXIT_FAILURE);
        }
    }

    if (wait (NULL) == -1)
    {
        syslog (LOG_ERR, "call to wait(2) failed: %s", strerror (errno));
        exit (EXIT_FAILURE);
    }
    
    write_to_utmp_wtpm_atexit ();
    exit (EXIT_SUCCESS);
}
