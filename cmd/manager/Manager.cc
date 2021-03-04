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

#include <sys/socket.h>
#include <sys/un.h>
#include <csignal>
#include <cstdio>
#include <unistd.h>

#include "Backend.hh"
#include "Manager.hh"
#include "eci/Event.hh"

Manager gMgr;

void Manager::init(int argc, char *argv[])
{
    int r = 0;
    char c;
    struct sockaddr_un sun;

    /* settings */
    const char *pathPersistentDb = NULL;
    const char *pathVolatileDb = NULL;
    const char *pathSocket;
    bool recreatePersistentDb = false;
    bool readOnly = false;
    bool systemMode = false;

#define SetIf(condition)                                                       \
    if (condition == -1)                                                       \
        r = -1;

    SetIf(loop.init());

    SetIf(loop.addSignal(this, SIGINT));
    SetIf(loop.addSignal(this, SIGUSR1));

    if (r == -1)
        die("Failed to initialise runloop.\n");

    /**
     * -c: delete any existing database at the given persistent DB path, and
     * create a new one instead. Do not try to start any targets. Used to create
     * a seed repository.
     * -o: start ready, only try to go into read-write mode if later requested
     * -p <path>: permanent db path
     * -q <path>: volatile db path
     * -r: try reattachment (if not, any volatile DB at given path is deleted
     * and recreated)
     * -s: system mode (start readonly - try to open read-write later. UNLESS
     * also reattaching - then we assume read-write unless directed otherwise)
     * -t <path>: path at which to create the listener socket
     */

    while ((c = getopt(argc, argv, "cp:q:rst:")) != -1)
        switch (c)
        {
        case 'c':
            recreatePersistentDb = true;
            break;
        case 'p':
            pathPersistentDb = optarg;
            break;
        case 'q':
            pathVolatileDb = optarg;
            break;
        case 'r':
            reattaching = true;
            break;
        case 's':
            systemMode = true;
            readOnly = true;
            break;
        case 't':
            pathSocket = optarg;
            break;
        case '?':
            if (optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            abort();
        }

    if (!pathSocket)
        die("No path given for listener socket.\n");
    if (!pathPersistentDb)
        die("No path given for persistent repository.\n");
    if (!pathVolatileDb && reattaching)
        die("Asked to reattach, but no path given for volatile repository.\n");
    if (!pathVolatileDb && !readOnly)
        die("No path given for volatile repository.\n");

    if (systemMode)
        readOnly = true;

    /* delete any old ECID socket */
    unlink(pathSocket);

    if ((listenFD = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        edie(errno, "Failed to create listener socket");

    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, pathSocket, sizeof(sun.sun_path));

#ifdef ECI_PLAT_HPUX

    if (bind(listenFD, (struct sockaddr *)&sun, sizeof(struct sockaddr_un)) ==
        -1)
#else
    if (bind(listenFD, (struct sockaddr *)&sun, SUN_LEN(&sun)) == -1)
#endif
        edie(errno, "Failed to bind listener socket");

    if (listen(listenFD, 10) == -1)
        edie(errno, "Failed to listen on listener socket");

    r = loop.addFD(this, listenFD, POLLIN | POLLHUP);
    if (r != 0)
        edie(-r, "Failed to add listener socket to the event loop");

    listener.attach(listenFD);

    bend.init(pathPersistentDb, pathVolatileDb, readOnly, recreatePersistentDb,
              reattaching);
}

void Manager::run()
{
    while (shouldRun)
    {
        loop.loop(NULL);
        usleep(10000);
    }
}

void Manager::fdEvent(EventLoop *loop, int fd, int revents)
{
    printf("FD Event: %d %d\n", fd, revents);
    listener.fdEvent(fd, revents);
}

void Manager::signalEvent(EventLoop *loop, int signum)
{
    if (signum == SIGINT)
        shouldRun = false;
    else
        printf("Got signal %d\n", signum);
}

void Manager::clientConnected(WSRPCTransport *xprt)
{
    int r = loop.addFD(this, xprt->fd, POLLIN | POLLHUP);
    if (r != 0)
        loge(kErr, -r, "Failed to add event source for new client FD %d",
             xprt->fd);
}

void Manager::clientDisconnected(WSRPCTransport *xprt)
{
    int r = loop.delFD(xprt->fd);
    if (r != 0)
        loge(kErr, -r,
             "Failed to delete event source for disconnected client FD %d\n",
             xprt->fd);
}

int main(int argc, char *argv[])
{
    gMgr.init(argc, argv);
    gMgr.run();
}