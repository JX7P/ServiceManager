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

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "eci/Event.hh"
#include "eci/Logger.hh"
#include "eci/Platform.h"
#include "io.eComCloud.eci.IManager.hh"

struct LsSys : Logger, io_eComCloud_eci_IManagerDelegate, Handler
{
    LsSys() : Logger("lssys"){};
    int main(int argc, char *argv[]);
};

int LsSys::main(int argc, char *argv[])
{
    char c;
    int fd;
    struct sockaddr_un sun;
    const char *pathSockManager = NULL;
    WSRPCTransport xprt;
    EventLoop loop;

    /* -t <path>: path of the sys.manager socket */
    while ((c = getopt(argc, argv, "t:")) != -1)
        switch (c)
        {
        case 't':
            pathSockManager = optarg;
            break;
        }

    if (!pathSockManager)
        die("Path to manager socket not specified.\n");

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to open socket");
        exit(1);
    }

    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, pathSockManager, sizeof(sun.sun_path));

#ifdef ECI_PLAT_HPUX
    if (connect(fd, (struct sockaddr *)&sun, sizeof(struct sockaddr_un)) == -1)
#else
    if (connect(fd, (struct sockaddr *)&sun, SUN_LEN(&sun)) == -1)
#endif
        edie(errno, "Failed to connect to manager socket at %s",
             pathSockManager);

    xprt.attach(fd);

    loop.init();
    loop.addFD(this, fd, POLLIN | POLLHUP);

    auto comp =
        io_eComCloud_eci_IManagerClnt::subscribe_v1_async(&xprt, this, 2);

    loop.loop(NULL);

    xprt.readyForRead();

    return fd;
}

int main(int argc, char *argv[])
{
    LsSys lssys;
    return lssys.main(argc, argv);
}