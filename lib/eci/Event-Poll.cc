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

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "eci/Core.h"
#include "eci/Event.hh"

struct TimerEntry
{
    bool valid : 1;
    bool fired : 1;
    TimerID timer;
};

/* Self-pipe. We write to this to wake up poll() after we get signalled. */
static int sigPipe[2];

/* Did any signal fire since last we checked? */
bool signalFired;

/* Did a particular signal number fire since last we checked? */
static bool signalsFired[NSIG];

/* We allow 255 timers per event loop. */
static TimerEntry timers[255];

void EventLoop::sigHandler(int signum, siginfo_t *siginfo, void *ctx)
{
    int savedErrno = errno;
    printf("Sig %d val %d!\n", signum, siginfo->si_value);
    if (signum == SIGALRM)
        timers[siginfo->si_value.sival_int].fired = true;
    signalFired = true;
    signalsFired[signum] = true;
    if (write(sigPipe[1], ".", 1) == -1)
        printf("FAILED TO WRITE TO SIGNAL SELFPIPE!! %s\n", strerror(errno));
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
        log(kErr, "Maximum number of timers (255) exceeded.\n");
        return -E2BIG;
    }

    its.it_interval = {0, 0};
    its.it_value = *ts;

    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGALRM;
    sigev.sigev_value.sival_int = entryId;

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

    /* only need one at first, for our selfpipe */
    pFDs = (struct pollfd *)malloc(sizeof *pFDs);
    if (!pFDs)
    {
        loge(kErr, errno, "Failed to allocate pollfd array");
        return -errno;
    }
    nPFDs = 1;
    pFDs[0].fd = sigPipe[0];
    pFDs[0].events = POLLIN;
    pFDs[0].revents = 0;
}

int EventLoop::loop(struct timespec *ts)
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
                if (i == SIGALRM)
                {
                    /* timer went off */
                    printf("TIMER WENT OFF!\n");

                    continue;
                }

                /* something other than a timer went off, therefore let us
                 * dispatch */
                printf("Signal %d fired\n", i);
            }

runpoll:
    if (!haveRunPoll)
        r = poll(pFDs, nPFDs, timeSpecToMSecs(ts));
    haveRunPoll = true;

    if (r == -1)
        if (errno == EINTR)
        {
            /* GNU/Linux ignores SA_RESTART for poll() for whatever reason. Let
             * us therefore return to poll() and it will probably return at once
             * with POLLIN on the self-pipe. */
            haveRunPoll = false;
            goto runpoll;
        }
        else
            loge(kWarn, errno, "Poll returned -1");

    if (pFDs[0].revents)
    {
        assert(pFDs[0].revents = POLLIN);
        pFDs[0].revents = 0; /* signal selfpipe written to */
    }

    for (int i = 1; i < nPFDs; i++)
        if (pFDs[i].revents)
        {
            printf("Revent on FD %d\n", pFDs[i].fd);
            pFDs[i].revents = 0;
        }

    /* can happen */
    if (signalFired)
        goto sigfired;

    return r;
}