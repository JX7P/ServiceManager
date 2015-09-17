#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/procctl.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <kvm.h>
#elif defined(__linux__)
#include <proc/readproc.h>
#include <sys/prctl.h>
#endif

#include "s16rr.h"

/* It is forbidden to include s16.h in this file.
 * Doing so leads to a FreeBSD-internal header conflict! */
void * s16mem_alloc (unsigned long);
void s16mem_free (void *);

int subreap_acquire ()
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
    return procctl (P_PID, getpid (), PROC_REAP_ACQUIRE, NULL);
#elif defined(__linux__)
    return prctl (PR_SET_CHILD_SUBREAPER, 1);
#endif
}

int subreap_relinquish ()
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
    return procctl (P_PID, getpid (), PROC_REAP_RELEASE, NULL);
#elif defined(__linux__)
    return prctl (PR_SET_CHILD_SUBREAPER, 0);
#endif
}

int subreap_status ()
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
    struct procctl_reaper_status pctl;
    procctl (P_PID, getpid (), PROC_REAP_STATUS, &pctl);
    return (pctl.rs_flags & REAPER_STATUS_OWNED);
#elif defined(__linux__)
    int status;
    prctl (PR_GET_CHILD_SUBREAPER, &status);
    return status;
#endif
}

process_wait_t * process_fork_wait (const char * cmd_)
{
    process_wait_t * pwait = s16mem_alloc (sizeof (process_wait_t));
    int n_spaces = 0;
    char *cmd = strdup (cmd_), *tofree = cmd, **argv = NULL;
    pid_t newPid;

    strtok (cmd, " ");

    while (cmd)
    {
        argv = (char **)realloc (argv, sizeof (char *) * ++n_spaces);
        argv[n_spaces - 1] = cmd;
        cmd = strtok (NULL, " ");
    }

    argv = (char **)realloc (argv, sizeof (char *) * (n_spaces + 1));
    argv[n_spaces] = 0;

    pipe (pwait->fd);
    newPid = fork ();

    if (newPid == 0) /* child */
    {
        char dispose;
        close (pwait->fd[1]);
        read (pwait->fd[0], &dispose, 1);
        execvp (argv[0], argv);
        exit (999);
    }
    else if (newPid < 0) /* fail */
    {
        fprintf (stderr, "FAILED TO FORK\n");
        pwait->pid = 0;
    }
    else
    {
        close (pwait->fd[0]);
        pwait->pid = newPid;
    }

    free (argv);
    free (tofree);

    return pwait;
}

void process_fork_continue (process_wait_t * pwait)
{
    write (pwait->fd[1], "0", 1);
    close (pwait->fd[1]);
    s16mem_free (pwait);
}

int exit_was_abnormal (int wstat)
{
    if (WIFEXITED (wstat))
        return WEXITSTATUS (wstat);
    else if (WIFSIGNALED (wstat))
    {
        int sig = WTERMSIG (wstat);
        if ((sig == SIGPIPE) || (sig == SIGTERM) || (sig == SIGHUP) ||
            (sig == SIGINT))
            return 0;
        else
            return sig;
    }
    else
        return 1;
}

void discard_signal (int no) { no = no; }

/* Borrowed from Epoch: http://universe2.us/epoch.html */
pid_t read_pid_file (const char * path)
{
    FILE * PIDFileDescriptor = fopen (path, "r");
    char PIDBuf[200], *TW = NULL, *TW2 = NULL;
    unsigned long Inc = 0;
    pid_t InPID;

    if (!PIDFileDescriptor)
    {
        return 0; // Zero for failure.
    }

    for (int TChar;
         (TChar = getc (PIDFileDescriptor)) != EOF && Inc < (200 - 1); ++Inc)
    {
        *(unsigned char *)&PIDBuf[Inc] = (unsigned char)TChar;
    }

    PIDBuf[Inc] = '\0';
    fclose (PIDFileDescriptor);

    for (TW = PIDBuf; *TW == '\n' || *TW == '\t' || *TW == ' '; ++TW)
        ;
    for (TW2 = TW; *TW2 != '\0' && *TW2 != '\t' && *TW2 != '\n' &&
                   *TW2 != '%' && *TW2 != ' ';
         ++TW2)
        ; // Delete any following the number.

    *TW2 = '\0';
    InPID = strtol (TW, 0, 10);
    return InPID;
}
