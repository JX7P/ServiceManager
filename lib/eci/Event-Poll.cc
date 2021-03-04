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
    TimerDesc timer;
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
    printf("Sig %d val %d!\n", signum, siginfo->si_value.sival_int);
    if (signum == SIGALRM)
        timers[siginfo->si_value.sival_int].fired = true;
    signalFired = true;
    signalsFired[signum] = true;
    if (write(sigPipe[1], ".", 1) == -1)
        printf("FAILED TO WRITE TO SIGNAL SELFPIPE!! %s\n", strerror(errno));
    errno = savedErrno;
}

int EventLoop::addFD(Handler *handler, int fd, int events)
{
    void *newPFDs = realloc(pFDs, sizeof(*pFDs) * ++nPFDs);

    if (!newPFDs)
    {
        nPFDs--;
        return -errno;
    }

    try
    {
        fdSources.emplace_back(handler, fd);
    }
    catch (std::bad_alloc)
    {
        loge(kErr, ENOMEM, "Failed to allocate FD source descriptor");
        nPFDs--;
        return -ENOMEM;
    }

    pFDs = (struct pollfd *)newPFDs;
    pFDs[nPFDs - 1].fd = fd;
    pFDs[nPFDs - 1].events = events;
    pFDs[nPFDs - 1].revents = 0;

    return 0;
}

int EventLoop::addSignal(Handler *handler, int sigNum)
{
    struct sigaction sigact;
    int r;

    try
    {
        sigSources.emplace_back(handler, sigNum);
    }
    catch (std::bad_alloc)
    {
        loge(kErr, ENOMEM, "Failed to allocate signal source descriptor");
        nPFDs--;
        return -ENOMEM;
    }

    sigact.sa_sigaction = sigHandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO | SA_RESTART;

    r = sigaction(sigNum, &sigact, (struct sigaction *)NULL);

    if (r == -1)
    {
        loge(kErr, errno, "Failed to add sigaction");
        sigSources.pop_back();
        return -errno;
    }

    return 0;
}

int EventLoop::addTimer(Handler *handler, struct timespec *ts)
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

    try
    {
        timerSources.emplace_back(handler, entryId);
    }
    catch (std::bad_alloc)
    {
        loge(kErr, ENOMEM, "Failed to allocate timer source descriptor");
        return -ENOMEM;
    }

    its.it_interval = {0, 0};
    its.it_value = *ts;

    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGALRM;
    sigev.sigev_value.sival_int = entryId;

    r = timer_create(CLOCK_REALTIME, &sigev, &timers[entryId].timer);
    if (r == -1)
    {
        int oldErrno = errno;
        loge(kErr, errno, "Error creating POSIX timer %d\n", entryId);
        timerSources.pop_back();
        return -oldErrno;
    }

    timers[entryId].fired = false;

    r = timer_settime(timers[entryId].timer, 0, &its, NULL);
    if (r == -1)
    {
        int oldErrno = errno;
        loge(kErr, errno, "Error setting time of POSIX timer %d\n", entryId);
        timerSources.pop_back();

        if (timer_delete(timers[entryId].timer) == -1)
            loge(kWarn, errno, "Error deleting POSIX timer %d", entryId);

        return -oldErrno;
    }

    timers[entryId].valid = true;

    return entryId;
}

int EventLoop::delFD(int fd)
{
    bool found = false;

    for (auto it = fdSources.begin(); it != fdSources.end(); it++)
        if (it->fd == fd)
        {
            found = true;
            fdSources.erase(it);
            break;
        }

    if (!found)
        log(kWarn,
            "No source descriptor found for FD %d - "
            "trying to find and delete pollfd entry anyway\n",
            fd);

    for (int i = 0; i < nPFDs; i++)
        if (pFDs[i].fd == fd)
        {
            memmove(&pFDs[i], &pFDs[i + 1], sizeof(*pFDs) * (nPFDs - i));
            pFDs--;
            return 0;
        }
    return -ENOENT;
}

int EventLoop::delSignal(int sigNum)
{
    log(kWarn, "Deleting signal events is not implemented\n");
    return -EBADF;
}

int EventLoop::delTimer(int entryID)
{
    int r;
    bool found = false;

    for (auto it = timerSources.begin(); it != timerSources.end(); it++)
        if (it->id == entryID)
        {
            found = true;
            timerSources.erase(it);
            break;
        }

    if (!found)
        log(kWarn,
            "No source descriptor found for timer entry %d - "
            "trying to delete POSIX timer anyway\n",
            entryID);

    timers[entryID].fired = false;
    timers[entryID].valid = false;
    r = timer_delete(timers[entryID].timer);
    timers[entryID].timer = 0;

    if (r == -1)
        loge(kWarn, errno, "Error deleting POSIX timer (entry %d)", entryID);

    return -ENOENT;
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

    r = addSignal(NULL, SIGALRM);
    if (r < 0)
        return r;

    /* only need one at first, for our selfpipe */
    pFDs = (struct pollfd *)malloc(sizeof *pFDs);
    if (!pFDs)
    {
        int oldErrno = errno;
        loge(kErr, errno, "Failed to allocate pollfd array");
        return -oldErrno;
    }
    nPFDs = 1;
    pFDs[0].fd = sigPipe[0];
    pFDs[0].events = POLLIN;
    pFDs[0].revents = 0;

    return 0;
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
                bool handled;

                /* FIXME: atomic */
                signalsFired[i] = false;
                handled = false;

                if (i == SIGALRM)
                {
                    for (int timID = 0; timID < 255; timID++)
                        if (timers[timID].valid && timers[timID].fired)
                        {
                            timers[timID].fired = false;
                            timers[timID].valid = false;
                            timers[timID].timer = 0;
                            for (auto it = timerSources.begin();
                                 it != timerSources.end(); it++)
                            {
                                if (it->id == timID)
                                {
                                    it->handler->timerEvent(this, timID);
                                    timerSources.erase(it);
                                    handled = true;
                                    break;
                                }
                            }
                            if (!handled)
                                log(kWarn,
                                    "Did not find a source descriptor for "
                                    "timer entry %d\n",
                                    timID);
                            continue;
                        }

                    continue;
                }

                /* something other than a timer went off, therefore let us
                 * dispatch */
                printf("Signal %d fired\n", i);
                for (auto it = sigSources.begin(); it != sigSources.end(); it++)
                {
                    if (it->sigNum == i)
                    {
                        it->handler->signalEvent(this, i);
                        handled = true;
                        continue;
                    }
                }

                if (!handled)
                    log(kWarn,
                        "Did not find a signal descriptor for signal %d\n", i);
            }

runpoll:
    if (!haveRunPoll)
        r = poll(pFDs, nPFDs, timeSpecToMSecs(ts));
    haveRunPoll = true;

    if (r == -1)
    {
        if (errno == EINTR)
        {
            /* GNU/Linux ignores SA_RESTART for poll() for whatever reason.
             * Let us therefore return to poll() and it will probably return
             * at once with POLLIN on the self-pipe. */
            haveRunPoll = false;
            goto runpoll;
        }
        else
            loge(kWarn, errno, "Poll returned -1");
    }

    if (pFDs[0].revents)
    {
        assert(pFDs[0].revents = POLLIN);
        pFDs[0].revents = 0; /* signal selfpipe written to */
    }

    for (int i = 1; i < nPFDs; i++)
        if (pFDs[i].revents)
        {
            int fd = pFDs[i].fd;

            for (auto it = fdSources.begin(); it != fdSources.end(); it++)
                if (it->fd == fd)
                {
                    it->handler->fdEvent(this, fd, pFDs[i].revents);
                    goto proceed3;
                }

            log(kWarn, "Did not find a source descriptor for FD %d\n", fd);

        proceed3:
            pFDs[i].revents = 0;
        }

    /* can happen */
    if (signalFired)
        goto sigfired;

    return r;
}