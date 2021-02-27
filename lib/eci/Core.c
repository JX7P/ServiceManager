/*******************************************************************

    PROPRIETARY NOTICE

These coded instructions, statements, and computer programs contain
proprietary information of eComCloud Object Solutions, and they are
protected under copyright law. They may not be distributed, copied,
or used except under the provisions of the terms of the Source Code
Licence Agreement, in the file "LICENCE.md", which should have been
included with this software

    Copyright Notice

    (c) 2015-2021 eComCloud Object Solutions.
        All rights reserved.
********************************************************************/

#include <sys/signal.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "eci/Core.h"

int eciCloseOnExec(int fd)
{
    int flags;
    flags = fcntl(fd, F_GETFD);
    if (flags != -1)
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
    else
    {
        perror("Fcntl!");
    }
}

ECIPendingProcess *eciProcessForkAndWait(const char *cmd_,
                                         void (*cleanup_cb)(void *),
                                         void *cleanup_cb_arg)
{
    ECIPendingProcess *pwait = malloc(sizeof(ECIPendingProcess));
    int n_spaces = 0;
    char *cmd = strdup(cmd_), *tofree = cmd, **argv = NULL, *saveptr = NULL;
    pid_t newPid;

    strtok_r(cmd, " ", &saveptr);

    while (cmd)
    {
        argv = (char **)realloc(argv, sizeof(char *) * ++n_spaces);
        argv[n_spaces - 1] = cmd;
        cmd = strtok_r(NULL, " ", &saveptr);
    }

    argv = (char **)realloc(argv, sizeof(char *) * (n_spaces + 1));
    argv[n_spaces] = 0;

    pipe(pwait->fd);
    newPid = fork();

    if (newPid == 0) /* child */
    {
        char dispose;
        close(pwait->fd[1]);
        read(pwait->fd[0], &dispose, 1);
        close(pwait->fd[0]);
        cleanup_cb(cleanup_cb_arg);
        execvp(argv[0], argv);
        perror("Execvp failed!");
        printf("Command: %s\n", cmd_);
        exit(999);
    }
    else if (newPid < 0) /* fail */
    {
        fprintf(stderr, "FAILED TO FORK\n");
        pwait->pid = 0;
    }
    else
    {
        close(pwait->fd[0]);
        pwait->pid = newPid;
    }

    free(argv);
    free(tofree);

    return pwait;
}

void eciPendingProcessContinue(ECIPendingProcess *pwait)
{
    write(pwait->fd[1], "0", 1);
    close(pwait->fd[1]);
    free(pwait);
}

int eciExitWasAbnormal(int wstat)
{
    if (WIFEXITED(wstat))
        return WEXITSTATUS(wstat);
    else if (WIFSIGNALED(wstat))
    {
        int sig = WTERMSIG(wstat);
        if ((sig == SIGPIPE) || (sig == SIGTERM) || (sig == SIGHUP) ||
            (sig == SIGINT))
            return 0;
        else
            return sig;
    }
    else
        return 1;
}