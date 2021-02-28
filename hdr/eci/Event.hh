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

#include <sys/poll.h>

#include "eci/Logger.hh"
#include "eci/Platform.h"

#if defined(ECI_EVENT_DRIVER_Poll)
#include <sys/signal.h>
#include <time.h>

typedef timer_t TimerDesc;
#elif defined(ECI_EVENT_DRIVER_EPoll)
#warning EPoll Driver
#elif defined(ECI_EVENT_DRIVER_KQueue)

typedef int TimerDesc;
#endif

class EventLoop;

class Handler
{
    friend class EventLoop;
    void fdEvent(EventLoop *loop, int fd, int revents){};
    void timerEvent(EventLoop *loop, int id){};
    void signalEvent(EventLoop *loop, int signum){};
};

class EventLoop : Logger
{
#if defined(ECI_EVENT_DRIVER_Poll)
#warning Poll driver
    int nPFDs;
    struct pollfd *pFDs;

    static void sigHandler(int signum, siginfo_t *siginfo, void *ctx);
#elif defined(ECI_EVENT_DRIVER_EPoll)
#warning EPoll Driver
    int epollFD;
#elif defined(ECI_EVENT_DRIVER_KQueue)
#warning KQueue
    int kqFD;
#endif

    struct Source
    {
        Handler *handler;
        Source(Handler *hdlr) : handler(hdlr){};
    };

    struct FDSource : public Source
    {
        int fd;
        FDSource(Handler *hdlr, int fd) : Source(hdlr), fd(fd){};
    };

    struct SignalSource : public Source
    {
        int sigNum;
        SignalSource(Handler *hdlr, int sig) : Source(hdlr), sigNum(sig){};
    };

    struct TimerSource : public Source
    {
        int id;
        TimerSource(Handler *hdlr, int entryID) : Source(hdlr), id(entryID){};
    };

    std::list<FDSource> fdSources;
    std::list<SignalSource> sigSources;
    std::list<TimerSource> timerSources;

  public:
    EventLoop(Logger *parent = NULL) : Logger("evloop", parent){};

    /**
     *  Begin monitoring an FD for events.
     *
     * @param events A bitset of events to listen for.
     * Supported events are POLLIN and POLLOUT.
     *
     * @returns -errno if unsuccessful.
     */
    int addFD(Handler *handler, int fd, int events);

    /**
     * Handle a signal with the event loop.
     *
     * @returns -errno if unsuccessful.
     */
    int addSignal(Handler *handler, int signum);

    /**
     * Adds a timer to go off in \param ts time.
     *
     * @returns A unique timer ID (>= 0) if successful.
     * @returns -errno if unsuccessful.
     */
    int addTimer(Handler *handler, struct timespec *ts);

    /**
     * Deletes the given FD from the watch list.
     *
     * Returns -ENOENT if the given FD is not in the watch list.
     */
    int delFD(int fd);

    int delTimer(int entryID);

    int delSignal(int sigNum);

    /**
     * Initialise this event loop.
     *
     * @returns -errno if unsuccessful.
     */
    int init();

    /**
     * Begin an event-loop iteration for a max of \p ts time.
     * If \param ts is a NULL pointer, waits indefinitely for an event.
     * If \param ts points to a zero-valued timespec, returns immediately.
     *
     * @returns -errno if an error is encountered
     * @returns 0 if timed out
     * @returns >0 if events were received and dispatched.
     */
    int loop(struct timespec *ts);
};