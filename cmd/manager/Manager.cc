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

#include <csignal>
#include <cstdio>

#include "Manager.hh"
#include "eci/Event.hh"

Manager gMgr;

void Manager::init()
{
}

void Manager::run()
{
}

int main()
{
    struct timespec ts;
    EventLoop loop;
    ts.tv_nsec = 999999999;
    ts.tv_sec = 1;
    loop.init();
    printf("Added timer %d\n", loop.addTimer(&ts));
    loop.addSignal(SIGUSR1);
    ts.tv_nsec = 999999999;
    ts.tv_sec = 3;
    loop.loop(&ts);
    gMgr.init();
    gMgr.run();
}