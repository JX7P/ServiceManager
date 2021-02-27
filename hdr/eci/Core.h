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

#include "eci/Platform.h"

typedef struct ECIPendingProcess
{
    int fd[2];
    pid_t pid;
} ECIPendingProcess;

/**
 * Forks a process for the command specified. This process waits until
 * eciPendingProcessContinue() is called.
 */
ECIPendingProcess *eciProcessForkAndWait(const char *cmd_,
                                         void (*cleanup_cb)(void *),
                                         void *cleanup_cb_arg);
/**
 * Tells the child process to continue and frees the ECIPendingProcess
 * structure.
 */
void eciPendingProcessContinue(ECIPendingProcess *pwait);

/**
 * Inspect the result of a wait() call. Returns 0 for a healthy exit, and either
 * signal number or return code if not.
 */
int eciExitWasAbnormal(int wstat);