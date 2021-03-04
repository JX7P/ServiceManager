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

int EventLoop::addFD(Handler *handler, int fd, int events)
{
    struct kevent ev;
    int r;
    bool addedRead = false;

    try
    {
        fdSources.emplace_back(handler, fd);
    }
    catch (std::bad_alloc)
    {
        loge(kErr, ENOMEM, "Failed to allocate FD source descriptor");
        return -ENOMEM;
    }

    if (events & POLLIN)
    {
        EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, 0, 0);

        if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
        {
            loge(Logger::kWarn, errno,
                 "Failed to add read event filter for FD %d\n", fd);
            fdSources.pop_back();
            return -errno;
        }

        addedRead = true;
    }
    if (events & POLLOUT)
    {
        EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD, 0, 0, 0);

        if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
        {
            loge(Logger::kWarn, errno,
                 "Failed to add write event filter for FD %d\n", fd);
            fdSources.pop_back();
            if (addedRead)
            {
                EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
                if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
                {
                    loge(Logger::kErr, errno,
                         "Failed to delete wread event filter for FD %d\n", fd);
                }
            }
            return -errno;
        }
    }

    return 0;
}

int EventLoop::addSignal(Handler *handler, int signum)
{
    struct kevent ev;
    int r;

    try
    {
        sigSources.emplace_back(handler, signum);
    }
    catch (std::bad_alloc)
    {
        loge(kErr, ENOMEM, "Failed to allocate signal source descriptor");
        return -ENOMEM;
    }

    EV_SET(&ev, signum, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);

    if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
    {
        loge(Logger::kWarn, errno, "Failed to add event filter for signal \n",
             signum);
        sigSources.pop_back();
        return -errno;
    }

    /**
     * On BSD, setting a signal event filter on a Kernel Queue does NOT
     * supersede ordinary signal disposition. Therefore we ignore the
     * signal; it'll be multiplexed into our event loop instead.
     */
    if (signal(signum, SIG_IGN) == SIG_ERR)
    {
        int olderrno = errno;
        loge(Logger::kWarn, errno, "Failed to ignore signal %d\n", signum);
        sigSources.pop_back();

        EV_SET(&ev, signum, EVFILT_SIGNAL, EV_DELETE, 0, 0, 0);

        if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
        {
            loge(Logger::kWarn, errno,
                 "Failed to delete event filter for signal \n", signum);
            sigSources.pop_back();
            return -errno;
        }

        return -olderrno;
    }
}

int EventLoop::addTimer(Handler *handler, struct timespec *ts)
{
    struct kevent ev;
    TimerDesc id = abs(rand());
    int r;

    try
    {
        timerSources.emplace_back(handler, id);
    }
    catch (std::bad_alloc)
    {
        loge(kErr, ENOMEM, "Failed to allocate timer source descriptor");
        return -ENOMEM;
    }

    EV_SET(&ev, id, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, timeSpecToMSecs(ts),
           0);
    r = kevent(kqFD, &ev, 1, NULL, 0, NULL);

    if (r == -1)
    {
        loge(Logger::kWarn, errno, "Failed to add event filter for timer");
        timerSources.pop_back();
        return -errno;
    }

    return id;
}

int EventLoop::delFD(int fd)
{
    bool found = false;
    bool succeeded = false;
    struct kevent ev;
    int r;
    std::list<FDSource>::iterator it;

    for (it = fdSources.begin(); it != fdSources.end(); it++)
    {
        if (it->fd == fd)
        {
            found = true;
            fdSources.erase(it);
            break;
        }
    }

    if (!found)
        log(kWarn,
            "No source descriptor found for FD %d - "
            "trying to delete KEvent filters anyway\n",
            fd);

    EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, 0);

    if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
        loge(Logger::kWarn, errno,
             "Failed to delete read event filter for FD %d\n", fd);
    else
        succeeded = true;

    EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);

    if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
        loge(Logger::kWarn, errno,
             "Failed to delete write event filter for FD %d\n", fd);
    else
        succeeded = true;

    return succeeded ? 0 : -ENOENT;
}

int EventLoop::delSignal(int sigNum)
{
    log(kWarn, "Deleting signal events is not implemented\n");
    return -EBADF;
}

int EventLoop::delTimer(int entryID)
{
    bool found = false;
    bool succeeded = false;
    int oldErrno;
    struct kevent ev;
    int r;
    std::list<TimerSource>::iterator it;

    for (it = timerSources.begin(); it != timerSources.end(); it++)
        if (it->id == entryID)
        {
            found = true;
            timerSources.erase(it);
            break;
        }

    if (!found)
        log(kWarn,
            "No source descriptor found for timer %d - "
            "trying to delete timer KEvent filter anyway\n",
            entryID);

    EV_SET(&ev, entryID, EVFILT_TIMER, EV_DELETE, 0, 0, 0);

    if (kevent(kqFD, &ev, 1, 0, 0, 0) == -1)
    {
        oldErrno = errno;
        loge(Logger::kWarn, errno,
             "Failed to delete timer event filter for timer %d", entryID);
        return -oldErrno;
    }

    return 0;
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
    case EVFILT_READ:
    case EVFILT_WRITE:
    {
        FDSource *src = NULL;

        for (auto it = fdSources.begin(); it != fdSources.end(); it++)
            if (it->fd == ev.ident)
            {
                src = &*it;
                break;
            }

        if (!src)
        {
            log(kWarn, "Did not find a source descriptor for FD %d\n",
                ev.ident);
            break;
        }

        if (ev.filter == EVFILT_READ)
        {
            if (ev.flags & EV_EOF)
                if (ev.data) /* data to be read: POLLIN + POLLHUP */
                    src->handler->fdEvent(this, ev.ident, 0 | POLLIN | POLLHUP);
                else /* no data to be read: POLLHUP only */
                    src->handler->fdEvent(this, ev.ident, 0 | POLLHUP);
            else
                src->handler->fdEvent(this, ev.ident, 0 | POLLIN);
        }
        else /* EVFILT_WRITE */
        {
            if (ev.flags & EV_EOF)
                /* I believe one only gets POLLHUP and not POLLOUT at once */
                src->handler->fdEvent(this, ev.ident, 0 | POLLHUP);
            else
                src->handler->fdEvent(this, ev.ident,
                                      0 | POLLOUT); /* POLLOUT only */
        }

        break;
    }

    case EVFILT_TIMER:
    {
        bool handled = false;

        for (auto it = timerSources.begin(); it != timerSources.end(); it++)
            if (it->id == ev.ident)
            {
                it->handler->timerEvent(this, ev.ident);
                /* clear the timer since our timers are oneshot affairs */
                timerSources.erase(it);
                handled = true;
                break;
            }

        if (!handled)
        {
            log(kWarn, "Did not find a source descriptor for timer %d\n",
                ev.ident);
            break;
        }

        break;
    }

    case EVFILT_SIGNAL:
    {
        bool handled = false;

        for (auto it = sigSources.begin(); it != sigSources.end(); it++)
        {
            if (it->sigNum == ev.ident)
            {
                it->handler->signalEvent(this, ev.ident);
                handled = true;
                break;
            }
        }

        if (!handled)
            log(kWarn, "Did not find a signal descriptor for signal %d\n",
                ev.ident);
        break;
    }
    default:
        log(kWarn, "Unhandled KEvent filter %d\n", ev.filter);
    }
    return 1;
}