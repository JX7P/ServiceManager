/*******************************************************************

    PROPRIETARY NOTICE

These coded instructions, statements, and computer programs contain
proprietary information of eComCloud Object Solutions, and they are
protected under copyright law. They may not be distributed, copied,
or used except under the provisions of the terms of the Source Code
Licence Agreement, in the file "LICENCE.md", which should have been
included with this software

    Copyright Notice

    (c) 2021 eComCloud Object Solutions.
        All rights reserved.
********************************************************************/

#include <sys/types.h>
#include <sys/poll.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "eci/Event.hh"

struct TimerEntry
{
    bool valid : 1;
    bool fired : 1;
    TimerID timer;
};

/* Self-pipe. We write to this to wake up poll() after we get signalled. */
static int sigPipe[2];

/* Did a signal fire during poll() ? */
bool signalFired;

/* We allow 255 timers per event loop. */
static TimerEntry timers[255];

/* For every signal, a boolean value indicating whether it fired or not. */
static bool signalsFired[NSIG];

void EventLoop::sigHandler(int signum, siginfo_t *siginfo, void *ctx)
{
    int savedErrno = errno;
    printf("Sig %d val %d!\n", signum, siginfo->si_value);
    if (signum == SIGALRM)
        timers[siginfo->si_value.sival_int].fired = true;
    signalsFired[signum] = true;
    if (write(sigPipe[1], ".", 1) == -1)
        printf("FAILED TO WRITE: %s\n", strerror(errno));
    errno = savedErrno;
}

int EventLoop::addFD(int fd)
{
    return 0;
}

int EventLoop::addSignal(int signum)
{
    struct sigaction sigact;
    int r;

    sigact.sa_sigaction = sigHandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO | SA_RESTART;

    r = sigaction(SIGALRM, &sigact, (struct sigaction *)NULL);

    if (r == -1)
        return -errno;
}

int EventLoop::addTimer(struct timespec *ts)
{
    struct sigevent sigev;
    struct itimerspec its;
    int entryId = -1;
    int r;

    for (int i = 0; i < 256; i++)
        if (!timers[i].valid)
        {
            entryId = i;
            break;
        }

    if (entryId == -1)
    {
        printf("Too many timers!\n");
        return -E2BIG;
    }

    its.it_interval = {0, 0};
    its.it_value = *ts;

    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGALRM;
    sigev.sigev_value.sival_int = 42; // entryId;

    r = timer_create(CLOCK_REALTIME, &sigev, &timers[entryId].timer);
    if (r == -1)
        return -errno;

    timers[entryId].fired = false;

    r = timer_settime(timers[entryId].timer, 0, &its, NULL);
    if (r == -1)
        return -errno;

    timers[entryId].valid = true;

    return entryId;
}

int EventLoop::init()
{
    int r;

    for (int i = 0; i < NSIG; i++)
        signalsFired[i] = false;

    for (int i = 0; i < 256; i++)
        timers[i].valid = false;

    if (pipe(sigPipe) == -1)
        return -errno;

    r = addSignal(SIGALRM);
    if (r < 0)
        return r;
}

int EventLoop::loop(int nMSecs)
{
    int r;
    bool oldSigFired;
    bool haveRunPoll = false;

    /* listen to sigPipe[0] for data */

/* in case signal fired outwith a poll iteration, not to wait forever */
sigfired:
    /* FIXME: Atomic ops */
    oldSigFired = signalFired;
    signalFired = false;

    if (oldSigFired)
        for (int i = 0; i < NSIG; i++)
            if (signalsFired[i] == true)
            {
                /* FIXME: atomic */
                signalsFired[i] = false;
                printf("Signal %d fired\n", i);
            }

    if (!haveRunPoll)
        r = poll(NULL, 0, -1);
    haveRunPoll = true;

    /* can happen */
    if (signalFired)
        goto sigfired;
}