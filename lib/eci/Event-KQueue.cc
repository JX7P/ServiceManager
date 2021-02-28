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
/**
 * A KQueue-based implementation of the simple event loop. This is only intended
 * for use with real Kernel Queues available on BSD platforms, not libkqueue.
 */

#include <sys/types.h>
#include <sys/event.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "eci/Core.h"
#include "eci/Event.hh"

/* Self-pipe. We write to this to wake up kevent() on user request. */
static int sigPipe[2];

int EventLoop::addFD(int fd)
{
    return 0;
}

int EventLoop::addSignal(int signum)
{
    struct kevent ev;
    int r;

    EV_SET(&ev, signum, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);

    if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
    {
        loge(Logger::kWarn, errno, "Failed to add event filter for signal \n",
             signum);
        return -errno;
    }
    /**
     * On BSD, setting a signal event filter on a Kernel Queue does NOT
     * supersede ordinary signal disposition. Therefore we ignore the signal;
     * it'll be multiplexed into our event loop instead.
     */
    if (signal(signum, SIG_IGN) == SIG_ERR)
        return -errno;
}

int EventLoop::addTimer(struct timespec *ts)
{
    struct kevent ev;
    TimerID id = abs(rand());
    int r;

    EV_SET(&ev, id, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, timeSpecToMSecs(ts),
           0);
    r = kevent(kqFD, &ev, 1, NULL, 0, NULL);

    if (r == -1)
    {
        loge(Logger::kWarn, errno, "Failed to add event filter for timer");
        return -errno;
    }

    return id;
}

int EventLoop::init()
{
    int r;

    kqFD = kqueue();

    if (kqFD == -1)
        return -errno;
}

int EventLoop::loop(struct timespec *ts)
{
    struct kevent ev;
    int r;

    r = kevent(kqFD, NULL, 0, &ev, 1, ts);
    if (r == -1)
    {
        loge(Logger::kWarn, errno, "KEvent wait failed");
        return -errno;
    }
    else if (!r)
    {
        return 0;
    }

    switch (ev.filter)
    {
    case EVFILT_TIMER:
        printf("Timer %d went off\n", ev.ident);
        break;
    case EVFILT_SIGNAL:
        printf("Signal %d arrived\n", ev.ident);
        break;
    }
    return 1;
}